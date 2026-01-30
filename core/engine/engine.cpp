#include <api/Globals.h>
#include "engine.h"
#include <api/LoggerListeners.h>
#include  <api/HttpClient/Sender.h>
#include "engine/app.h"
#include "engine/exchange.h"
#include "event/event.h"
#include "engine/utility.h"
#include "engine/settings.h"
#include "engine/converter.h"

using namespace std::placeholders;


namespace Keen
{
	namespace engine
	{
		std::vector<cLogger::cAttachment> g_logger_attachments;

		TradeEngine::TradeEngine(EventEmitter* event_emitter)
			: event_emitter(event_emitter)
		{
			this->event_emitter->start();

			//os.chdir(TRADER_DIR);    // Change working directory
			this->init_engines();
		}

		TradeEngine::~TradeEngine()
		{
			for (auto& [key, exchange] : exchanges)
			{
				exchange->close();
				delete exchange;
			}
			exchanges.clear();

			for (auto& [key, engine] : engines)
			{
				engine->close();
				delete engine;
			}
			engines.clear();

			for (auto& [key, app] : apps)
			{
				delete app;
			}
			apps.clear();

			exchanges.clear();
		}

		BaseEngine* TradeEngine::add_engine(BaseEngine* engine)
		{
			if (!engine) return nullptr;

			this->engines[engine->engine_name] = engine;
			return engine;
		}

		void TradeEngine::init_engines()
		{
			/*
			* Init all engines.
			*/
			this->add_engine(new LogEngine(this, this->event_emitter));
			this->add_engine(new OmsEngine(this, this->event_emitter));
			this->add_engine(new EmailEngine(this, this->event_emitter));
			this->add_engine(new NoticelEngine(this, this->event_emitter));

			this->send_order = [=, this](const OrderRequest& req, AString exchange_name)->AString
				{
					auto exchange = this->get_exchange(exchange_name);
					if (exchange)
						return exchange->send_order(req);
					else
						return "";
				};
		}

		void TradeEngine::write_log(AString msg, AString source/* = ""*/)
		{
			LogData log = { .msg = msg, .exchange_name = source };
			Event event = Event(EVENT_LOG, log);
			this->event_emitter->put(event);
		}

		BaseExchange* TradeEngine::get_exchange(AString exchange_name)
		{
			BaseExchange* exchange = GetWithNull(this->exchanges, exchange_name);
			if (exchange == nullptr)
				this->write_log("The interface could not be found:" + exchange_name);
			return exchange;
		}

		BaseEngine* TradeEngine::get_engine(AString engine_name)
		{
			BaseEngine* engine = GetWithNull(this->engines, engine_name);
			if (engine == nullptr)
				this->write_log("Engine not found:" + engine_name);
			return engine;
		}

		Json TradeEngine::get_default_setting(AString exchange_name)
		{
			BaseExchange* exchange = this->get_exchange(exchange_name);
			if (exchange)
				return exchange->get_default_setting();
			return Json();
		}

		AStringList TradeEngine::get_all_exchange_names()
		{
			AStringList names;
			for (auto it : this->exchanges)
			{
				names.push_back(it.first);
			}
			return names;
		}

		std::list<BaseApp*> TradeEngine::get_all_apps()
		{
			std::list<BaseApp*> apps;
			for (auto it : this->apps)
			{
				apps.push_back(it.second);
			}
			return apps;
		}

		std::list<Exchange> TradeEngine::get_all_exchanges()
		{
			std::list<Exchange> exchanges;
			for (auto it : this->exchanges)
			{
				exchanges.push_back(it.second->exchange);
			}
			return exchanges;
		}

		void TradeEngine::connect(const Json& setting, AString exchange_name)
		{
			if (setting.is_null())
				return;

			auto exchange = this->get_exchange(exchange_name);
			if (exchange)
			{
				try
				{
					exchange->connect(setting);
				}
				catch (const std::exception& e)
				{
					LOGERROR(Printf("connect to exchange:%s catch %s", exchange_name.c_str(), e.what()));
				}
			}
		}

		void TradeEngine::subscribe(const SubscribeRequest& req, AString exchange_name)
		{
			auto exchange = this->get_exchange(exchange_name);
			if (exchange)
				exchange->subscribe(req);
		}

		void TradeEngine::cancel_order(const CancelRequest& reqs, AString exchange_name)
		{
			auto exchange = this->get_exchange(exchange_name);
			if (exchange)
				exchange->cancel_order(reqs);
		}

		AString TradeEngine::send_quote(const QuoteRequest& req, AString exchange_name)
		{
			auto exchange = this->get_exchange(exchange_name);
			if (exchange)
				return exchange->send_quote(req);
			else
				return "";
		}


		void TradeEngine::cancel_quote(const CancelRequest& req, AString exchange_name)
		{
			auto exchange = this->get_exchange(exchange_name);
			if (exchange)
				exchange->cancel_quote(req);
		}


		std::list<BarData> TradeEngine::query_history(const HistoryRequest& req, AString exchange_name)
		{
			auto exchange = this->get_exchange(exchange_name);
			if (exchange)
				return exchange->query_history(req);
			else
				return {};
		}

		void TradeEngine::close()
		{
			this->event_emitter->stop();

			for (auto engine : this->engines)
			{
				engine.second->close();
			}

			for (auto exchange : this->exchanges)
			{
				exchange.second->close();
			}
		}




		BaseEngine::BaseEngine(
			TradeEngine* trade_engine,
			EventEmitter* event_emitter,
			AString engine_name)
		{
			this->trade_engine = trade_engine;
			this->event_emitter = event_emitter;
			this->engine_name = engine_name;
		}

		void BaseEngine::close()
		{

		}

		BaseEngine::~BaseEngine()
		{

		}


		LogEngine::LogEngine(TradeEngine* trade_engine, EventEmitter* event_emitter)
			: BaseEngine(trade_engine, event_emitter, "log")
		{

			// 			if (not SETTINGS["log.active"])
			// 				return;
			// 
			// 			this->level = SETTINGS["log.level"];
			// 
			// 			this->logger = logging.getLogger("Keen Trader");
			// 			this->logger.setLevel(this->level);
			// 
			// 			this->formatter = logging.Formatter(
			// 				"%(asctime)s  %(levelname)s: %(message)s"
			// 			);

			//			if (SETTINGS["log.console"])
			this->add_console_handler();

			//			if (SETTINGS["log.file"])
			this->add_file_handler();

			this->register_event();
		}

		LogEngine::~LogEngine()
		{

		}

		void LogEngine::add_console_handler()
		{
			auto consoleLogListener = MakeConsoleListener();
			g_logger_attachments.push_back(cLogger::GetInstance().AttachListener(std::move(consoleLogListener)));
		}

		void LogEngine::add_file_handler()
		{
			/*
			* Add file output of log.
			*/
			// 			today_date = datetime.now().strftime("%Y%m%d");
			// 			filename = f"kt_{today_date}.log";
			// 			log_path = get_folder_path("log");
			// 			file_path = log_path.joinpath(filename);
			// 
			// 			file_handler = logging.FileHandler(
			// 				file_path, mode = "a", encoding = "utf8"
			// 			);
			// 			file_handler.setLevel(this->level);
			// 			file_handler.setFormatter(this->formatter);
			// 			this->logger.addHandler(file_handler);

			auto fileLogListener = MakeFileListener();
			if (fileLogListener)
			{
				g_logger_attachments.push_back(cLogger::GetInstance().AttachListener(std::move(fileLogListener)));
			}
		}

		void LogEngine::register_event()
		{
			this->event_emitter->Register(EVENT_LOG, std::bind(&LogEngine::process_log_event, this, _1));
		}

		void LogEngine::process_log_event(const Event& event)
		{
			const LogData& log = std::any_cast<const LogData&>(event.data);
			cLogger::GetInstance().LogSimple(log.msg, /*log.level*/eLogLevel::Info);
		}



		OmsEngine::OmsEngine(TradeEngine* trade_engine, EventEmitter* event_emitter)
			: BaseEngine(trade_engine, event_emitter, "oms")
		{
			this->add_function();
			this->register_event();
		}

		OmsEngine::~OmsEngine()
		{

		}

		void OmsEngine::add_function()
		{
			this->trade_engine->get_tick = std::bind(&OmsEngine::get_tick, this, _1);
			this->trade_engine->get_order = std::bind(&OmsEngine::get_order, this, _1);
			this->trade_engine->get_trade = std::bind(&OmsEngine::get_trade, this, _1);
			this->trade_engine->get_position = std::bind(&OmsEngine::get_position, this, _1);
			this->trade_engine->get_account = std::bind(&OmsEngine::get_account, this, _1);
			this->trade_engine->get_contract = std::bind(&OmsEngine::get_contract, this, _1);
			this->trade_engine->get_quote = std::bind(&OmsEngine::get_quote, this, _1);
			this->trade_engine->get_all_ticks = std::bind(&OmsEngine::get_all_ticks, this);
			this->trade_engine->get_all_orders = std::bind(&OmsEngine::get_all_orders, this);
			this->trade_engine->get_all_trades = std::bind(&OmsEngine::get_all_trades, this);
			this->trade_engine->get_all_positions = std::bind(&OmsEngine::get_all_positions, this);
			this->trade_engine->get_all_accounts = std::bind(&OmsEngine::get_all_accounts, this);
			this->trade_engine->get_all_contracts = std::bind(&OmsEngine::get_all_contracts, this);
			this->trade_engine->get_all_quotes = std::bind(&OmsEngine::get_all_quotes, this);
			this->trade_engine->get_all_active_orders = std::bind(&OmsEngine::get_all_active_orders, this, _1);
			this->trade_engine->get_all_active_quotes = std::bind(&OmsEngine::get_all_active_quotes, this, _1);

			this->trade_engine->update_order_request = std::bind(&OmsEngine::update_order_request, this, _1, _2, _3);
			this->trade_engine->convert_order_request = std::bind(&OmsEngine::convert_order_request, this, _1, _2, _3, 4);
			this->trade_engine->get_converter = std::bind(&OmsEngine::get_converter, this, _1);
		}

		void OmsEngine::register_event()
		{
			this->event_emitter->Register(EVENT_TICK, std::bind(&OmsEngine::process_tick_event, this, _1));
			this->event_emitter->Register(EVENT_ORDER, std::bind(&OmsEngine::process_order_event, this, _1));
			this->event_emitter->Register(EVENT_TRADE, std::bind(&OmsEngine::process_trade_event, this, _1));
			this->event_emitter->Register(EVENT_POSITION, std::bind(&OmsEngine::process_position_event, this, _1));
			this->event_emitter->Register(EVENT_ACCOUNT, std::bind(&OmsEngine::process_account_event, this, _1));
			this->event_emitter->Register(EVENT_CONTRACT, std::bind(&OmsEngine::process_contract_event, this, _1));
			this->event_emitter->Register(EVENT_QUOTE, std::bind(&OmsEngine::process_quote_event, this, _1));
		}

		void OmsEngine::process_tick_event(const Event& event)
		{
			const TickData& tick = std::any_cast<const TickData&>(event.data);
			this->ticks[tick.kt_symbol] = tick;
		}

		void OmsEngine::process_order_event(const Event& event)
		{
			const OrderData& order = std::any_cast<const OrderData&>(event.data);
			this->orders[order.kt_orderid] = order;

			if (order.is_active())
			{
				this->active_orders[order.kt_orderid] = order;
			}
			else if (this->active_orders.contains(order.kt_orderid))
			{
				this->active_orders.erase(order.kt_orderid);
			}

			// Update to offset converter
			OffsetConverter* converter = GetWithNull(this->offset_converters, order.exchange_name);
			if (converter)
				converter->update_order(order);
		}

		void OmsEngine::process_trade_event(const Event& event)
		{
			const TradeData& trade = std::any_cast<const TradeData&>(event.data);
			this->trades[trade.kt_tradeid] = trade;

			// Update to offset converter
			OffsetConverter* converter = GetWithNull(this->offset_converters, trade.exchange_name);
			if (converter)
				converter->update_trade(trade);
		}

		void OmsEngine::process_position_event(const Event& event)
		{
			const PositionData& position = std::any_cast<const PositionData&>(event.data);
			this->positions[position.kt_positionid] = position;

			// Update to offset converter
			OffsetConverter* converter = GetWithNull(this->offset_converters, position.exchange_name);
			if (converter)
				converter->update_position(position);
		}

		void OmsEngine::process_account_event(const Event& event)
		{
			const AccountData& account = std::any_cast<const AccountData&>(event.data);
			this->accounts[account.kt_accountid] = account;
		}

		void OmsEngine::process_contract_event(const Event& event)
		{
			const ContractData& contract = std::any_cast<const ContractData&>(event.data);
			this->contracts[contract.kt_symbol] = contract;

			// Initialize offset converter for each exchange

			if (this->offset_converters.contains(contract.exchange_name))
				this->offset_converters[contract.exchange_name] = new OffsetConverter(this->trade_engine);
		}

		void OmsEngine::process_quote_event(const Event& event)
		{
			const QuoteData& quote = std::any_cast<const QuoteData&>(event.data);
			this->quotes[quote.kt_quoteid] = quote;

			// If quote is active, then update data in dict.
			if (quote.is_active())
				this->active_quotes[quote.kt_quoteid] = quote;
			// Otherwise, pop inactive quote from in dict
			else if (this->active_quotes.contains(quote.kt_quoteid))
				this->active_quotes.erase(quote.kt_quoteid);
		}

		std::optional<TickData> OmsEngine::get_tick(AString kt_symbol)
		{
			return GetWithNone(this->ticks, kt_symbol);
		}

		std::optional<OrderData> OmsEngine::get_order(AString kt_orderid)
		{
			return GetWithNone(this->orders, kt_orderid);
		}

		std::optional<TradeData> OmsEngine::get_trade(AString kt_tradeid)
		{
			return GetWithNone(this->trades, kt_tradeid);
		}

		std::optional<PositionData> OmsEngine::get_position(AString kt_positionid)
		{
			return GetWithNone(this->positions, kt_positionid);
		}

		std::optional<AccountData> OmsEngine::get_account(AString kt_accountid)
		{
			return GetWithNone(this->accounts, kt_accountid);
		}

		std::optional<ContractData> OmsEngine::get_contract(AString kt_symbol)
		{
			return GetWithNone(this->contracts, kt_symbol);
		}

		std::optional<QuoteData> OmsEngine::get_quote(AString kt_quoteid)
		{
			return GetWithNone(this->quotes, kt_quoteid);
		}

		std::list<TickData> OmsEngine::get_all_ticks()
		{
			return MapValues(this->ticks);
		}

		std::list<OrderData> OmsEngine::get_all_orders()
		{
			return MapValues(this->orders);
		}

		std::list<TradeData> OmsEngine::get_all_trades()
		{
			return MapValues(this->trades);
		}

		std::list<PositionData> OmsEngine::get_all_positions()
		{
			return MapValues(this->positions);
		}

		std::list<AccountData> OmsEngine::get_all_accounts()
		{
			return  MapValues(this->accounts);
		}

		std::list<ContractData> OmsEngine::get_all_contracts()
		{
			return  MapValues(this->contracts);
		}

		std::list<QuoteData> OmsEngine::get_all_quotes()
		{
			return MapValues(this->quotes);
		}

		std::list<OrderData> OmsEngine::get_all_active_orders(AString kt_symbol/* = ""*/)
		{
			if (kt_symbol.empty())
				return MapValues(this->active_orders);
			else
			{
				std::list<OrderData> t_active_orders;
				for (auto& [key, order] : this->active_orders)
				{
					if (order.kt_symbol == kt_symbol)
					{
						t_active_orders.push_back(order);
					}
				}

				return t_active_orders;
			}
		}

		std::list<QuoteData> OmsEngine::get_all_active_quotes(AString kt_symbol/* = ""*/)
		{
			if (kt_symbol.empty())
				return MapValues(this->active_quotes);
			else
			{
				std::list<QuoteData> t_active_quotes;
				for (auto& [key, quote] : this->active_quotes)
				{
					if (quote.kt_symbol == kt_symbol)
					{
						t_active_quotes.push_back(quote);
					}
				}

				return t_active_quotes;
			}
		}

		void OmsEngine::update_order_request(const OrderRequest& req, const AString& kt_orderid, const AString& exchange_name)
		{
			OffsetConverter* converter = GetWithNull(this->offset_converters, exchange_name);
			if (converter)
				converter->update_order_request(req, kt_orderid);
		}

		std::list<OrderRequest> OmsEngine::convert_order_request(const OrderRequest& req,
			const AString& exchange_name, bool lock, bool net/* = false*/)
		{
			OffsetConverter* converter = GetWithNull(this->offset_converters, exchange_name);
			if (!converter)
				return { req };

			std::list<OrderRequest> reqs = converter->convert_order_request(req, lock, net);
			return reqs;
		}

		OffsetConverter* OmsEngine::get_converter(const AString& exchange_name)
		{
			return GetWithNull(this->offset_converters, exchange_name);
		}



		EmailEngine::EmailEngine(TradeEngine* trade_engine, EventEmitter* event_emitter)
			: BaseEngine(trade_engine, event_emitter, "email")
		{
			this->_active = false;

			this->trade_engine->send_email = std::bind(&EmailEngine::send_email, this, _1, _2, _3);
		}

		void EmailEngine::send_email(AString subject, AString content, AString receiver /*= ""*/)
		{
			// Start email engine when sending first email.
			if (!this->_active)
				this->start();

			// Use voidault receiver if not specified.
			if (receiver.empty())
				receiver = SETTINGS["email.receiver"];

			EmailMessage msg;
			msg.subject = subject;
			msg.content = content;
			msg.receiver = receiver;

			cCSLock Lock(m_CS);
			this->_queue.push(msg);
			m_QueueNonempty.Set();
		}

		void EmailEngine::run()
		{
			for (;;)
			{
				cCSLock Lock(m_CS);
				while (this->_active && (this->_queue.size() == 0))
				{
					cCSUnlock Unlock(Lock);
					m_QueueNonempty.Wait();
				}
				if (!this->_active)
				{
					return;
				}

				ASSERT(!this->_queue.empty());

				EmailMessage& email_msg = this->_queue.front();

				this->trade_engine->write_log(email_msg.subject);

				//try
				//{
				//	// create mail message
				//	message msg;
				//	msg.from(mail_address(SETTINGS["email.sender"], SETTINGS["email.username"]));
				//	msg.add_recipient(mail_address(email_msg.receiver, email_msg.receiver));
				//	msg.subject(email_msg.subject);
				//	msg.content(email_msg.content);

				//	AString host = SETTINGS["email.server"];
				//	unsigned port = SETTINGS["email.port"];
				//	smtps conn(host, port);
				//	conn.authenticate(SETTINGS["email.username"], SETTINGS["email.password"], smtps::auth_method_t::LOGIN);
				//	conn.submit(msg);
				//}
				//catch (smtp_error& exc)
				//{
				//	this->trade_engine->write_log(exc.what());
				//}
				//catch (dialog_error& exc)
				//{
				//	this->trade_engine->write_log(exc.what());
				//}


				this->_queue.pop();
				Lock.Unlock();
			}
		}

		void EmailEngine::start()
		{
			this->_thread = std::thread(&EmailEngine::run, this);
			this->_active = true;
		}

		void EmailEngine::close()
		{
			if (!this->_active)
				return;

			m_QueueNonempty.Set();

			if (this->_thread.joinable())
			{
				this->_thread.join();
			}
		}



		//////////////////////////////////////////////////////////////////////
		NoticelEngine::NoticelEngine(TradeEngine* trade_engine, EventEmitter* event_emitter)
			: BaseEngine(trade_engine, event_emitter, "notice")
		{
			this->_active = false;
			this->_route = SETTINGS["notice.route"];
			this->_token = SETTINGS["notice.token"];
			this->_chat_id = SETTINGS["notice.chat_id"];

			this->trade_engine->send_notice = std::bind(&NoticelEngine::send_notice, this, _1, _2);
		}

		NoticelEngine::~NoticelEngine()
		{
	
		}

		void NoticelEngine::send_notice(AString subject, AString content)
		{
			// Start email engine when sending first email.
			if (!this->_active)
				this->start();


			AString	route = SETTINGS["notice.route"];

			NoticeMessage msg;
			msg.subject = subject;
			msg.content = content;
		

			cCSLock Lock(m_CS);
			this->_queue.push(msg);
			m_QueueNonempty.Set();
		}

		void NoticelEngine::run()
		{
			for (;;)
			{
				cCSLock Lock(m_CS);
				while (this->_active && (this->_queue.size() == 0))
				{
					cCSUnlock Unlock(Lock);
					m_QueueNonempty.Wait();
				}
				if (!this->_active)
				{
					return;
				}

				ASSERT(!this->_queue.empty());

				NoticeMessage& notice_msg = this->_queue.front();

				this->trade_engine->write_log(notice_msg.subject);

				static AString url_base = "https://api.telegram.org";
				AString path = Printf("/bot%s/sendMessage", this->_token.c_str());

				api::Params params = { 
					{"chat_id", this->_chat_id},
					{"text", notice_msg.content},
					{"parse_mode", "HTML"},
				};

				api::Request request = {
				.method = "POST",
				.path = path,
				.params = params
				};

				api::Response resp = api::_http_request(request, url_base);

				if (resp.status / 100 != 2)
				{
					AString msg = Printf("Failed to send notice, status code: %d, information: %s", resp.status,
						resp.body.c_str());
					this->trade_engine->write_log(msg);
				}
				else
				{
					Json data = Json::parse(resp.body); // get() is needed!

				}
	
				this->_queue.pop();
				Lock.Unlock();
			}
		}

		void NoticelEngine::start()
		{
			this->_thread = std::thread(&NoticelEngine::run, this);
			this->_active = true;
		}

		void NoticelEngine::close()
		{
			if (!this->_active)
				return;

			m_QueueNonempty.Set();

			if (this->_thread.joinable())
			{
				this->_thread.join();
			}
		}
	}
}

