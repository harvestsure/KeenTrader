#include <api/Globals.h>
#include "engine.h"
#include "template.h"
#include <filesystem>
#include <dylib.hpp>

#include <engine/converter.h>
#include <event/event.h>
#include <engine/utility.h>
#include <api/OSSupport/ThreadPool.h>

namespace fs = std::filesystem;
using namespace std::placeholders;


namespace Keen
{
	namespace app
	{
		static std::map<Status, StopOrderStatus> STOP_STATUS_MAP = {
			{Status::SUBMITTING, StopOrderStatus::WAITING},
			{Status::NOTTRADED, StopOrderStatus::WAITING},
			{Status::PARTTRADED, StopOrderStatus::TRIGGERED},
			{Status::ALLTRADED, StopOrderStatus::TRIGGERED},
			{Status::CANCELLED, StopOrderStatus::CANCELLED},
			{Status::REJECTED, StopOrderStatus::CANCELLED}};

		AString CtaEngine::setting_filename = "cta_strategy_setting.json";
		AString CtaEngine::data_filename = "cta_strategy_data.json";

		CtaEngine::CtaEngine(TradeEngine* trade_engine, EventEmitter* event_emitter)
			: BaseEngine(trade_engine, event_emitter, APP_NAME), engine_type(EngineType::LIVE), strategy_setting(Json::object()), strategy_data(Json::object())
		{
			init_executor = new ThreadPool(1);
		}

		CtaEngine::~CtaEngine()
		{
			for (auto class_ins : classes)
			{
				dylib *lib = std::any_cast<dylib *>(class_ins);
				delete lib;
			}

			classes.clear();

			delete init_executor;
			init_executor = nullptr;
		}

		void CtaEngine::init_engine()
		{
			this->init_datafeed();
			this->load_strategy_class();
			this->load_strategy_setting();
			this->load_strategy_data();
			this->register_event();
			this->write_log("CTA strategy engine initialized");
		}

		void CtaEngine::close()
		{
			this->stop_all_strategies();
		}

		void CtaEngine::register_event()
		{
			this->event_emitter->Register(EVENT_TICK, std::bind(&CtaEngine::process_tick_event, this, std::placeholders::_1));
			this->event_emitter->Register(EVENT_ORDER, std::bind(&CtaEngine::process_order_event, this, std::placeholders::_1));
			this->event_emitter->Register(EVENT_TRADE, std::bind(&CtaEngine::process_trade_event, this, std::placeholders::_1));
		}

		void CtaEngine::init_datafeed()
		{
			// Init RQData client.
			// 				bool result = rqdata_client.init();
			// 				if (result)
			// 					this->write_log("RQData data interface initialization is successful");
		}

		std::list<BarData> CtaEngine::query_bar_from_datafeed(AString symbol, Exchange exchange, Interval interval, DateTime start, DateTime end)
		{
			std::list<BarData> bars;
			// Query bar data from RQData.

			// 			HistoryRequest req = HistoryRequest(
			// 				symbol = symbol,
			// 				exchange = exchange,
			// 				interval = interval,
			// 				start = start,
			// 				end = end
			// 			);
			// 			data = rqdata_client.query_history(req);
			// 			return data;

			return bars;
		}

		void CtaEngine::process_tick_event(const Event &event)
		{
			const TickData &tick = std::any_cast<const TickData &>(event.data);

			std::list<CtaTemplate *> strategies = this->symbol_strategy_map[tick.kt_symbol];
			if (!strategies.size())
				return;

			this->check_stop_order(tick);

			for (CtaTemplate *strategy : strategies)
			{
				if (strategy->inited)
					strategy->on_tick(tick);
			}
		}

		void CtaEngine::process_order_event(const Event &event)
		{
			const OrderData &order = std::any_cast<const OrderData &>(event.data);

			CtaTemplate *strategy = GetWithNull(this->orderid_strategy_map, order.kt_orderid);
			if (not strategy)
				return;

			// Remove kt_orderid if order is no longer active.
			AStringSet &kt_orderids = this->strategy_orderid_map[strategy->strategy_name];
			if (kt_orderids.count(order.kt_orderid) && !order.is_active())
				kt_orderids.erase(order.kt_orderid);

			// For server stop order, call strategy on_stop_order function
			if (order.type == OrderType::STOP)
			{
				StopOrder so{
					.kt_symbol = order.kt_symbol,
					.direction = order.direction,
					.offset = order.offset,
					.price = order.price,
					.volume = order.volume,
					.stop_orderid = order.kt_orderid,
					.strategy_name = strategy->strategy_name,
					.datetime = order.datetime,
					.kt_orderids = {order.kt_orderid},
					.status = STOP_STATUS_MAP[order.status]};

				strategy->on_stop_order(so);
			}

			// Call strategy on_order function
			strategy->on_order(order);
		}

		void CtaEngine::process_trade_event(const Event &event)
		{
			const TradeData &trade = std::any_cast<const TradeData &>(event.data);

			// Filter duplicate trade push
			if (this->kt_tradeids.count(trade.kt_tradeid))
				return;
			this->kt_tradeids.insert(trade.kt_tradeid);

			CtaTemplate *strategy = GetWithNull(this->orderid_strategy_map, trade.kt_orderid);
			if (!strategy)
				return;

			// Update strategy pos before calling on_trade method
			if (trade.direction == Direction::LONG)
				strategy->pos += trade.volume;
			else
				strategy->pos -= trade.volume;

			strategy->on_trade(trade);

			// Sync strategy variables to data file
			this->sync_strategy_data(strategy);

			// Update GUI
			this->put_strategy_event(strategy);
		}

		void CtaEngine::check_stop_order(const TickData &tick)
		{
			AStringMap removeMap;

			for (auto &[key, stop_order] : this->stop_orders)
			{
				if (stop_order.kt_symbol != tick.kt_symbol)
					continue;

				bool long_triggered = (stop_order.direction == Direction::LONG && tick.last_price >= stop_order.price);
				bool short_triggered = (stop_order.direction == Direction::SHORT && tick.last_price <= stop_order.price);

				if (long_triggered || short_triggered)
				{
					CtaTemplate *strategy = this->strategies[stop_order.strategy_name];

					// To get excuted immediately after stop order is
					// triggered, use limit price if available, otherwise
					// use ask_price_5 or bid_price_5
					float price = 0;
					if (stop_order.direction == Direction::LONG)
					{
						if (tick.limit_up)
							price = tick.limit_up;
						else
							price = tick.ask_price_5;
					}
					else
					{
						if (tick.limit_down)
							price = tick.limit_down;
						else
							price = tick.bid_price_5;
					}

					std::optional<ContractData> contract = this->trade_engine->get_contract(stop_order.kt_symbol);

					AStringList kt_orderids = this->send_limit_order(
						strategy,
						contract.value(),
						stop_order.direction,
						stop_order.offset,
						price,
						stop_order.volume,
						stop_order.lock,
						stop_order.net);

					// Update stop order status if placed successfully
					if (kt_orderids.size())
					{
						// Remove from relation map.
						this->stop_orders.erase(stop_order.stop_orderid);

						AStringSet &strategy_kt_orderids = this->strategy_orderid_map[strategy->strategy_name];
						if (strategy_kt_orderids.count(stop_order.stop_orderid))
							strategy_kt_orderids.erase(stop_order.stop_orderid);

						// Change stop order status to cancelled && update to strategy->
						stop_order.status = StopOrderStatus::TRIGGERED;
						stop_order.kt_orderids = kt_orderids;

						strategy->on_stop_order(stop_order);
						this->put_stop_order_event(stop_order);
					}
				}
			}
		}

		AStringList CtaEngine::send_server_order(
			CtaTemplate *strategy,
			ContractData contract,
			Direction direction,
			Offset offset,
			float price,
			float volume,
			OrderType type,
			bool lock,
			bool net)
		{
			// Create request && send order.
			OrderRequest original_req{
				.symbol = contract.symbol,
				.exchange = contract.exchange,
				.direction = direction,
				.type = type,
				.volume = volume,
				.price = price,
				.offset = offset,
				.reference = Printf("{APP_NAME}_{strategy.strategy_name}")};
			original_req.__post_init__();

			// Convert with offset converter
			std::list<OrderRequest> req_list = this->trade_engine->convert_order_request(original_req,
																						contract.exchange_name, lock, net);

			// Send Orders
			AStringList kt_orderids;

			for (const OrderRequest &req : req_list)
			{
				AString kt_orderid = this->trade_engine->send_order(req, contract.exchange_name);

				// Check if sending order successful
				if (kt_orderid.empty())
					continue;

				kt_orderids.push_back(kt_orderid);

				this->trade_engine->update_order_request(req, kt_orderid, contract.exchange_name);

				// Save relationship between orderid && strategy->
				this->orderid_strategy_map[kt_orderid] = strategy;
				this->strategy_orderid_map[strategy->strategy_name].insert(kt_orderid);
			}

			return kt_orderids;
		}

		AStringList CtaEngine::send_limit_order(
			CtaTemplate *strategy,
			ContractData contract,
			Direction direction,
			Offset offset,
			float price,
			float volume,
			bool lock,
			bool net)
		{
			/*
			 * Send a limit order to server.
			 */
			return this->send_server_order(
				strategy,
				contract,
				direction,
				offset,
				price,
				volume,
				OrderType::LIMIT,
				lock,
				net);
		}

		AStringList CtaEngine::send_server_stop_order(
			CtaTemplate *strategy,
			ContractData contract,
			Direction direction,
			Offset offset,
			float price,
			float volume,
			bool lock,
			bool net)
		{
			// Send a stop order to server.

			// Should only be used if stop order supported
			// on the trading server.
			return this->send_server_order(
				strategy,
				contract,
				direction,
				offset,
				price,
				volume,
				OrderType::STOP,
				lock,
				net);
		}

		AStringList CtaEngine::send_local_stop_order(
			CtaTemplate *strategy,
			Direction direction,
			Offset offset,
			float price,
			float volume,
			bool lock,
			bool net)
		{
			/*
			 * Create a new local stop order.
			 */
			this->stop_order_count += 1;
			AString stop_orderid = Printf("%s.%d", STOPORDER_PREFIX.c_str(), this->stop_order_count);

			StopOrder stop_order{
				.kt_symbol = strategy->kt_symbol,
				.direction = direction,
				.offset = offset,
				.price = price,
				.volume = volume,
				.stop_orderid = stop_orderid,
				.strategy_name = strategy->strategy_name,
				.lock = lock,
				.net = net};

			this->stop_orders[stop_orderid] = stop_order;

			AStringSet &kt_orderids = this->strategy_orderid_map[strategy->strategy_name];
			kt_orderids.insert(stop_orderid);

			strategy->on_stop_order(stop_order);
			this->put_stop_order_event(stop_order);

			return {stop_orderid};
		}

		void CtaEngine::cancel_server_order(CtaTemplate *strategy, AString kt_orderid)
		{
			/*
			 * Cancel existing order by kt_orderid.
			 */
			std::optional<OrderData> order = this->trade_engine->get_order(kt_orderid);
			if (!order)
			{
				this->write_log(Printf("Failed to cancel the order, can't find the entrustment %s", kt_orderid.c_str()), strategy);
				return;
			}

			CancelRequest req = order->create_cancel_request();
			this->trade_engine->cancel_order(req, order->exchange_name);
		}

		void CtaEngine::cancel_local_stop_order(CtaTemplate *strategy, AString stop_orderid)
		{
			/*
			 * Cancel a local stop order.
			 */
			if (!this->stop_orders.count(stop_orderid))
				return;

			StopOrder &stop_order = this->stop_orders.at(stop_orderid);

			strategy = this->strategies[stop_order.strategy_name];

			// Remove from relation map.
			this->stop_orders.erase(stop_orderid);

			AStringSet &kt_orderids = this->strategy_orderid_map[strategy->strategy_name];
			if (kt_orderids.count(stop_orderid))
				kt_orderids.erase(stop_orderid);

			// Change stop order status to cancelled && update to strategy->
			stop_order.status = StopOrderStatus::CANCELLED;

			strategy->on_stop_order(stop_order);
			this->put_stop_order_event(stop_order);
		}

		AStringList CtaEngine::send_order(
			CtaTemplate *strategy,
			Direction direction,
			Offset offset,
			float price,
			float volume,
			bool stop,
			bool lock,
			bool net)
		{
			auto finder = this->trade_engine->get_contract(strategy->kt_symbol);
			if (!finder)
			{
				this->write_log(Printf("Commission failed, contract not found: %s", strategy->kt_symbol.c_str()), strategy);
				return {};
			}

			ContractData contract = finder.value();

			// Round order price && volume to nearest incremental value
			price = round_to(price, contract.pricetick);
			volume = round_to(volume, contract.min_volume);

			if (stop)
			{
				if (contract.stop_supported)
					return this->send_server_stop_order(strategy, contract, direction, offset, price, volume, lock, net);
				else
					return this->send_local_stop_order(strategy, direction, offset, price, volume, lock, net);
			}
			else
			{
				return this->send_limit_order(strategy, contract, direction, offset, price, volume, lock, net);
			}
		}

		void CtaEngine::cancel_order(CtaTemplate *strategy, AString kt_orderid)
		{
			if (kt_orderid.find(STOPORDER_PREFIX) == 0)
				this->cancel_local_stop_order(strategy, kt_orderid);
			else
				this->cancel_server_order(strategy, kt_orderid);
		}

		void CtaEngine::cancel_all(CtaTemplate *strategy)
		{
			/*
			 * Cancel all active orders of a strategy->
			 */
			AStringSet kt_orderids = this->strategy_orderid_map[strategy->strategy_name];
			if (!kt_orderids.size())
				return;

			for (AString kt_orderid : kt_orderids)
			{
				this->cancel_order(strategy, kt_orderid);
			}
		}

		EngineType CtaEngine::get_engine_type()
		{
			return this->engine_type;
		}

		float CtaEngine::get_pricetick(CtaTemplate *strategy)
		{
			std::optional<ContractData> contract = this->trade_engine->get_contract(strategy->kt_symbol);

			if (contract)
				return contract->pricetick;
			else
				return 0;
		}

		int CtaEngine::get_size(CtaTemplate *strategy)
		{
			std::optional<ContractData> contract = this->trade_engine->get_contract(strategy->kt_symbol);

			if (contract)
				return contract->size;
			else
				return 0;
		}

		std::list<BarData> CtaEngine::load_bar(AString kt_symbol, int days, Interval interval,
											   FnMut<void(BarData)> callback /* = nullptr*/, bool use_database /* = false*/)
		{
			auto [symbol, exchange] = extract_kt_symbol(kt_symbol);

			DateTime end = currentDateTime();
			DateTime start = end - std::chrono::hours(days * 24);

			std::list<BarData> bars;

			if (!use_database)
			{
				std::optional<ContractData> contract = this->trade_engine->get_contract(kt_symbol);
				if (contract && contract->history_data)
				{
					HistoryRequest req{
						.symbol = symbol,
						.exchange = exchange,
						.start = start,
						.end = end,
						.interval = interval};
					req.__post_init__();

					bars = this->trade_engine->query_history(req, contract->exchange_name);
				}
				// Try to query bars from RQData, if not found, load from database.
				else
				{
					bars = this->query_bar_from_datafeed(symbol, exchange, interval, start, end);
				}
			}

			return bars;
		}

		void CtaEngine::call_strategy_func(CtaTemplate *strategy, FnMut<void(std::any)> func, std::any params)
		{
			/*
			 * Call function of a strategy && catch any exception raised.
			 */
			try
			{
				func(params);
			}
			catch (std::exception ex)
			{
				strategy->trading = false;
				strategy->inited = false;

				AString msg = Printf("Triggering exception has stopped\n%s", ex.what());
				this->write_log(msg, strategy);
			}
		}

		void CtaEngine::add_strategy(AString class_name, AString strategy_name, AString kt_symbol, Json setting)
		{
			/*
			 * Add a new strategy->
			 */
			if (this->strategies.count(strategy_name))
			{
				this->write_log(Printf("Failed to create policy, duplicate name %s exists", strategy_name.c_str()));
				return;
			}

			if (this->classes.find(class_name) == this->classes.end())
			{
				this->write_log(Printf("Failed to create policy, policy class %s not found", class_name.c_str()));
				return;
			}

			if (kt_symbol.find(".") == std::string::npos)
			{
				this->write_log("Failed to create strategy, local code is missing exchange suffix");
				return;
			}

			dylib *lib = std::any_cast<dylib *>(this->classes[class_name]);

			auto pGetStrategyInstance = lib->get_function<CtaTemplate *(CtaEngine *, const char *, const char *, Json)>(StrategyInstance);
			CtaTemplate *strategy = pGetStrategyInstance(this, strategy_name.c_str(), kt_symbol.c_str(), setting);

			this->strategies[strategy_name] = strategy;

			// Add kt_symbol to strategy map.
			std::list<CtaTemplate *> &strategies = this->symbol_strategy_map[kt_symbol];
			strategies.push_back(strategy);

			// Update to setting file.
			this->update_strategy_setting(strategy_name, setting);

			this->put_strategy_event(strategy);
		}

		void CtaEngine::init_strategy(AString strategy_name)
		{
			/*
			 * Init a strategy.
			 */
			// this->init_executor->enqueue(std::bind(&CtaEngine::_init_strategy, this, _1), strategy_name);
			this->_init_strategy(strategy_name);
		}

		void CtaEngine::_init_strategy(AString strategy_name)
		{
			/*
			 * Init strategies in queue.
			 */
			CtaTemplate *strategy = this->strategies[strategy_name];

			if (strategy->inited)
			{
				this->write_log(Printf("%s initialization has been completed. Repeated operation is prohibited.", strategy_name.c_str()));
				return;
			}

			this->write_log(Printf("%s starts initialization", strategy_name.c_str()));

			// Call on_init function of strategy
			strategy->on_init();

			// Restore strategy data(variables)
			Json data = this->strategy_data.value(strategy_name, Json());
			if (data.is_object())
			{
				for (AString name : strategy->variables)
				{
					Json value = data.value(name, Json());
					if (value.is_object())
					{
						// setattr(strategy, name, value);
					}
				}
			}

			// Subscribe market data
			std::optional<ContractData> contract = this->trade_engine->get_contract(strategy->kt_symbol);
			if (contract)
			{
				SubscribeRequest req{
					.symbol = contract->symbol,
					.exchange = contract->exchange};
				req.__post_init__();

				this->trade_engine->subscribe(req, contract->exchange_name);
			}
			else
			{
				this->write_log(Printf("Market subscription failed, contract %s not found", strategy->kt_symbol.c_str()));
			}

			// Put event to update init completed status.
			strategy->inited = true;
			this->put_strategy_event(strategy);
			this->write_log(Printf("%s Initialization completed", strategy_name.c_str()));
		}

		void CtaEngine::start_strategy(AString strategy_name)
		{
			/*
			 * Start a strategy
			 */
			CtaTemplate *strategy = this->strategies[strategy_name];
			if (!strategy->inited)
			{
				this->write_log(Printf("Strategy %s failed to start, please initialize first", strategy->strategy_name.c_str()));
				return;
			}

			if (strategy->trading)
			{
				this->write_log(Printf("%s has been started, please do not repeat the operation", strategy_name.c_str()));
				return;
			}

			strategy->on_start();
			strategy->trading = true;

			this->put_strategy_event(strategy);
		}

		void CtaEngine::stop_strategy(AString strategy_name)
		{
			/*
			 * Stop a strategy
			 */
			CtaTemplate *strategy = this->strategies[strategy_name];
			if (!strategy->trading)
				return;

			// Call on_stop function of the strategy
			strategy->on_stop();

			// Change trading status of strategy to false
			strategy->trading = false;

			// Cancel all orders of the strategy
			this->cancel_all(strategy);

			// Sync strategy variables to data file
			this->sync_strategy_data(strategy);

			// Update GUI
			this->put_strategy_event(strategy);
		}

		void CtaEngine::edit_strategy(AString strategy_name, Json setting)
		{
			/*
			 * Edit parameters of a strategy->
			 */
			CtaTemplate *strategy = this->strategies[strategy_name];
			strategy->update_setting(setting);

			this->update_strategy_setting(strategy_name, setting);
			this->put_strategy_event(strategy);
		}

		bool CtaEngine::remove_strategy(AString strategy_name)
		{
			/*
			 * Remove a strategy->
			 */
			CtaTemplate *strategy = this->strategies[strategy_name];
			if (strategy->trading)
			{
				this->write_log(Printf("strategy %s removal failed, please stop first", strategy->strategy_name.c_str()));
				return false;
			}

			// Remove setting
			this->remove_strategy_setting(strategy_name);

			// Remove from symbol strategy map
			std::list<CtaTemplate *> &strategies = this->symbol_strategy_map[strategy->kt_symbol];
			strategies.remove(strategy);

			// Remove from active orderid map
			if (this->strategy_orderid_map.count(strategy_name))
			{
				AStringSet kt_orderids = this->strategy_orderid_map.at(strategy_name);

				// Remove kt_orderid strategy map
				for (AString kt_orderid : kt_orderids)
				{
					if (this->orderid_strategy_map.count(kt_orderid))
						this->orderid_strategy_map.erase(kt_orderid);
				}

				this->strategy_orderid_map.erase(strategy_name);
			}
			// Remove from strategies
			this->strategies.erase(strategy_name);

			return true;
		}

		void CtaEngine::load_strategy_class()
		{
			/*
			 * Load strategy class from source code.
			 */
			auto current_path = fs::current_path();
			current_path.append("strategies");
			if (!fs::exists(current_path))
			{
				fs::create_directory(current_path);
			}

			this->load_strategy_class_from_folder(current_path.string());
		}

		void CtaEngine::load_strategy_class_from_folder(AString root_path)
		{
			/*
			 * Load strategy class from certain folder.
			 */
			for (const auto &entry : fs::directory_iterator(root_path))
			{
				if (entry.is_directory())
				{
					continue;
				}

				fs::path file_path = entry.path();

				AString ext = file_path.extension().string();
				if (ext.compare(".dll") && ext.compare(".so"))
				{
					continue;
				}

				AString strategy_module_name = file_path.string();
				this->load_strategy_class_from_module(strategy_module_name);
			}
		}

		void CtaEngine::load_strategy_class_from_module(AString module_name)
		{
			/*
			 * Load strategy class from module file.
			 */
			try
			{
				dylib *lib = new dylib(module_name, false);

				if (lib->has_symbol(StrategyClass) && lib->has_symbol(StrategyInstance))
				{
					auto pGetStrategyClass = lib->get_function<const char *()>(StrategyClass);
					AString strategy_class = pGetStrategyClass();

					this->classes[strategy_class] = lib;
				}
			}
			catch (const std::exception &ex)
			{
				AString msg = Printf("Policy file %s failed to load, triggering an exception:\n%s", module_name.c_str(), ex.what());
				this->write_log(msg);
			}
		}

		void CtaEngine::load_strategy_data()
		{
			/*
			 * Load strategy data from json file.
			 */
			this->strategy_data = load_json(this->data_filename);
		}

		void CtaEngine::sync_strategy_data(CtaTemplate *strategy)
		{
			/*
			 * Sync strategy data into json file.
			 */
			Json data = strategy->get_variables();
			data.erase("inited"); // Strategy status(inited, trading) should not be synced.
			data.erase("trading");

			this->strategy_data[strategy->strategy_name] = data;
			save_json(this->data_filename, this->strategy_data);
		}

		AStringList CtaEngine::get_all_strategy_class_names()
		{
			//
			// Return names of strategy classes loaded.
			//
			return MapKeys(this->classes);
		}

		void CtaEngine::get_strategy_class_parameters(AString class_name)
		{
			//
			// Get default parameters of a strategy class.
			//
			// 			strategy_class = this->classes[class_name];
			//
			// 			parameters = {};
			// 			for name in strategy_class.parameters :
			// 			parameters[name] = getattr(strategy_class, name);
			//
			// 			return parameters;
		}

		void CtaEngine::get_strategy_parameters(AString strategy_name)
		{
			//
			// Get parameters of a strategy->
			//
			CtaTemplate *strategy = this->strategies[strategy_name];
			return strategy->get_parameters();
		}

		void CtaEngine::init_all_strategies()
		{
			for (auto &[strategy_name, strategy] : this->strategies)
			{
				this->init_strategy(strategy_name);
			}
		}

		void CtaEngine::start_all_strategies()
		{
			for (auto &[strategy_name, strategy] : this->strategies)
				this->start_strategy(strategy_name);
		}

		void CtaEngine::stop_all_strategies()
		{
			for (auto &[strategy_name, strategy] : this->strategies)
				this->stop_strategy(strategy_name);
		}

		void CtaEngine::load_strategy_setting()
		{
			//
			// Load setting file.
			//
			this->strategy_setting = load_json(this->setting_filename);

			for (auto &[strategy_name, strategy_config] : this->strategy_setting.items())
			{
				this->add_strategy(
					strategy_config["class_name"],
					strategy_name,
					strategy_config["kt_symbol"],
					strategy_config["setting"]);
			}
		}

		void CtaEngine::update_strategy_setting(AString strategy_name, Json setting)
		{
			/*
			 * Update setting file.
			 */
			CtaTemplate *strategy = this->strategies[strategy_name];

			this->strategy_setting[strategy_name] =
				{
					{"class_name", strategy->__class__()},
					{"kt_symbol", strategy->kt_symbol},
					{"setting", setting}};
			save_json(this->setting_filename, this->strategy_setting);
		}

		void CtaEngine::remove_strategy_setting(AString strategy_name)
		{
			//
			// Update setting file.
			//
			if (!this->strategy_setting.count(strategy_name))
				return;

			this->strategy_setting.erase(strategy_name);
			save_json(this->setting_filename, this->strategy_setting);
		}

		void CtaEngine::put_stop_order_event(StopOrder stop_order)
		{
			//
			// Put an event to update stop order status.
			//
			Event event = Event(EVENT_CTA_STOPORDER, stop_order);
			this->event_emitter->put(event);
		}

		void CtaEngine::put_strategy_event(CtaTemplate *strategy)
		{
			//
			// Put an event to update strategy status.
			//
			Json data = strategy->get_data();
			Event event = Event(EVENT_CTA_STRATEGY, data);
			this->event_emitter->put(event);
		}

		void CtaEngine::write_log(AString msg, CtaTemplate *strategy /* = nullptr*/)
		{
			/*
			 * Create cta engine log event.
			 */
			if (strategy)
				msg = Printf("%s: %s", strategy->strategy_name.c_str(), msg.c_str());

			LogData log{.msg = msg, .exchange_name = "CtaStrategy"};
			Event event = Event(EVENT_CTA_LOG, log);
			this->event_emitter->put(event);
		}

		void CtaEngine::send_email(AString msg, CtaTemplate *strategy /* = nullptr*/)
		{
			//
			// Send email to default receiver.
			//
			AString subject;
			if (strategy)
				subject = strategy->strategy_name;
			else
				subject = "CTA策略引擎";

			this->trade_engine->send_email(subject, msg, "");
		}
	}
}
