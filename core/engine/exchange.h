#pragma once

#include "engine/object.h"

namespace Keen
{
	namespace engine
	{
		class Event;
		class EventEmitter;

		class KEEN_ENGINE_EXPORT BaseExchange
		{
			friend class TradeEngine;
		public:
			BaseExchange(EventEmitter* event_emitter, AString exchange_name);
			virtual ~BaseExchange();

			virtual void on_event(AString type, std::any data = nullptr);

			virtual void on_tick(const TickData& tick);

			virtual void on_trade(const TradeData& trade);

			virtual void on_order(const OrderData& order);

			virtual void on_position(const PositionData& position);

			virtual void on_account(const AccountData& account);

			virtual void on_quote(const QuoteData& quote);

			virtual void on_log(const LogData& log);

			virtual void on_contract(const ContractData& contract);

			virtual void write_log(const AString& msg);

			virtual void connect(const Json& setting) = 0;

			virtual void close() = 0;

			virtual void subscribe(const SubscribeRequest& req) = 0;

			virtual AString send_order(const OrderRequest& req);

			virtual void cancel_order(const CancelRequest& req);

			virtual AString send_quote(const QuoteRequest& req);

			virtual void cancel_quote(const CancelRequest& req);

			virtual void query_account() = 0;

			virtual void query_position() = 0;

			virtual std::list<BarData> query_history(const HistoryRequest& req);

			virtual const Json& get_default_setting() const;

			AString get_exchange_name() const { return this->exchange_name; }

		protected:
			Json default_setting;

			Exchange exchange;

			EventEmitter* event_emitter;
			AString exchange_name;
		};
	}
}
