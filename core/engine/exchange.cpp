#include <api/Globals.h>
#include <engine/engine.h>
#include "exchange.h"
#include "event/event.h"
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

		void BaseExchange::cancel_order(const CancelRequest& req)
		{

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

		const Json& BaseExchange::get_default_setting() const
		{
			return this->default_setting;
		}


	}
}
