#include <api/Globals.h>
#include <engine/engine.h>
#include "exchange.h"
#include "engine/event.h"
#include "engine/utility.h"

using namespace std::placeholders;


namespace Keen
{
	namespace engine
	{
		BaseExchange::BaseExchange(EventEmitter* event_emitter, AString exchange_name)
		{
			this->event_emitter = event_emitter;
			this->exchange_name = exchange_name;
		}

		BaseExchange::~BaseExchange()
		{

		}

		void BaseExchange::on_event(AString type, std::any data/* = nullptr*/)
		{
			Event event = Event(type, data);
			this->event_emitter->put(event);
		}


		void BaseExchange::on_tick(const TickData& tick)
		{
			this->on_event(EVENT_TICK, tick);
			this->on_event(EVENT_TICK + tick.kt_symbol, tick);
		}

		void BaseExchange::on_trade(const TradeData& trade)
		{
			this->on_event(EVENT_TRADE, trade);
			this->on_event(EVENT_TRADE + trade.kt_symbol, trade);
		}

		void BaseExchange::on_order(const OrderData& order)
		{
			this->on_event(EVENT_ORDER, order);
			this->on_event(EVENT_ORDER + order.kt_orderid, order);
		}

		void BaseExchange::on_position(const PositionData& position)
		{
			this->on_event(EVENT_POSITION, position);
			this->on_event(EVENT_POSITION + position.kt_symbol, position);
		}

		void BaseExchange::on_account(const AccountData& account)
		{
			this->on_event(EVENT_ACCOUNT, account);
			this->on_event(EVENT_ACCOUNT + account.kt_accountid, account);
		}

		void BaseExchange::on_quote(const QuoteData& quote)
		{
			this->on_event(EVENT_QUOTE, quote);
			this->on_event(EVENT_QUOTE + quote.kt_symbol, quote);
		}

		void BaseExchange::on_log(const LogData& log)
		{
			this->on_event(EVENT_LOG, log);
		}

		void BaseExchange::on_contract(const ContractData& contract)
		{
			this->on_event(EVENT_CONTRACT, contract);
		}

		void BaseExchange::write_log(const AString& msg)
		{
			LogData log = { .msg = msg, .exchange_name = this->exchange_name };
			this->on_log(log);
		}

		AString BaseExchange::send_order(const OrderRequest& req)
		{
			return "";
		}

		AString BaseExchange::send_quote(const QuoteRequest& req)
		{
			return "";
		}

		void BaseExchange::cancel_quote(const CancelRequest& req)
		{

		}

		std::list<BarData> BaseExchange::query_history(const HistoryRequest& req)
		{
			return {};
		}

		Json BaseExchange::get_default_setting()
		{
			return this->default_setting;
		}


		LocalOrderManager::LocalOrderManager(BaseExchange* exchange, AString order_prefix /*= ""*/)
		{
			this->exchange = exchange;

			// For generating local orderid
			this->order_prefix = order_prefix;
			this->order_count = 0;

			this->_cancel_order = exchange->cancel_order;
			exchange->cancel_order = std::bind(&LocalOrderManager::cancel_order, this, _1);
		}

		AString LocalOrderManager::new_local_orderid()
		{
			this->order_count += 1;
			AString local_orderid = this->order_prefix + Printf("%08d", this->order_count);
			return local_orderid;
		}

		AString LocalOrderManager::get_local_orderid(AString sys_orderid)
		{
			AString local_orderid;

			auto it = this->sys_local_orderid_map.find(sys_orderid);
			if (it != this->sys_local_orderid_map.end())
			{
				local_orderid = it->second;
			}
			else
			{
				local_orderid = this->new_local_orderid();
				this->update_orderid_map(local_orderid, sys_orderid);
			}

			return local_orderid;
		}

		AString LocalOrderManager::get_sys_orderid(AString local_orderid)
		{
			auto it = this->local_sys_orderid_map.find(local_orderid);
			if (it != this->local_sys_orderid_map.end())
			{
				return it->second;
			}
			else
			{
				return "";
			}
		}

		void LocalOrderManager::update_orderid_map(AString local_orderid, AString sys_orderid)
		{
			this->sys_local_orderid_map[sys_orderid] = local_orderid;
			this->local_sys_orderid_map[local_orderid] = sys_orderid;

			this->check_cancel_request(local_orderid);
			this->check_push_data(sys_orderid);
		}

		void LocalOrderManager::check_push_data(AString sys_orderid)
		{
			/*
			* Check if any order push data waiting.
			*/
			if (!this->push_data_buf.contains(sys_orderid))
				return;

			Json data = this->push_data_buf[sys_orderid];
			this->push_data_buf.erase(sys_orderid);

			if (this->push_data_callback)
				this->push_data_callback(data);
		}

		void LocalOrderManager::add_push_data(AString sys_orderid, Json data)
		{
			this->push_data_buf[sys_orderid] = data;
		}

		std::optional<OrderData> LocalOrderManager::get_order_with_sys_orderid(AString sys_orderid)
		{
			auto local_orderid = GetWithNone(this->sys_local_orderid_map, sys_orderid);
			auto it = this->sys_local_orderid_map.find(sys_orderid);
			if (!local_orderid)
				return std::nullopt;
			else
				return this->get_order_with_local_orderid(local_orderid.value());
		}

		OrderData LocalOrderManager::get_order_with_local_orderid(AString local_orderid)
		{
			OrderData order = this->orders[local_orderid];
			return copy(order);
		}

		void LocalOrderManager::on_order(OrderData order)
		{
			this->orders[order.orderid] = copy(order);
			this->exchange->on_order(order);
		}

		void LocalOrderManager::cancel_order(CancelRequest req)
		{
			AString sys_orderid = this->get_sys_orderid(req.orderid);
			if (sys_orderid.empty())
			{
				this->cancel_request_buf[req.orderid] = req;
				return;
			}

			this->_cancel_order(req);
		}

		void LocalOrderManager::check_cancel_request(AString local_orderid)
		{
			if (!this->cancel_request_buf.contains(local_orderid))
				return;

			CancelRequest req = this->cancel_request_buf[local_orderid];
			this->cancel_request_buf.erase(local_orderid);
			this->exchange->cancel_order(req);
		}
	}
}
