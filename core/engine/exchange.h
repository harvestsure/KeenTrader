#pragma once

#include "engine/object.h"

namespace Keen
{
	namespace engine
	{
		class Event;
		class EventEmitter;

		class KEEN_TRADER_EXPORT BaseExchange
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

			virtual AString  send_quote(const QuoteRequest& req);

			virtual void cancel_quote(const CancelRequest& req);

			virtual void query_account() = 0;

			virtual void query_position() = 0;

			virtual std::list<BarData> query_history(const HistoryRequest& req);

			virtual Json get_default_setting();

			FnMut<void(const CancelRequest&)> cancel_order;

		protected:
			Json default_setting;

			Exchange exchange;

			EventEmitter* event_emitter;
			AString exchange_name;
		};


		class KEEN_TRADER_EXPORT LocalOrderManager
		{
		public:
			LocalOrderManager(BaseExchange* exchange, AString order_prefix = "");

			AString new_local_orderid();

			AString get_local_orderid(AString sys_orderid);

			AString get_sys_orderid(AString local_orderid);

			void update_orderid_map(AString local_orderid, AString sys_orderid);

			void check_push_data(AString sys_orderid);

			void add_push_data(AString sys_orderid, Json data);

			std::optional<OrderData> get_order_with_sys_orderid(AString sys_orderid);

			OrderData get_order_with_local_orderid(AString local_orderid);

			void on_order(OrderData order);

			void cancel_order(CancelRequest req);

			void check_cancel_request(AString local_orderid);

		protected:

			BaseExchange* exchange;

			AString order_prefix;
			int order_count;
			std::map<AString, OrderData> orders;	// local_orderid:order

			// Map between local and system orderid
			std::map<AString, AString> local_sys_orderid_map;
			std::map<AString, AString> sys_local_orderid_map;

			// Push order data buf
			std::map<AString, Json> push_data_buf;  // sys_orderid:data

			// Callback for processing push order data
			FnMut<void(Json)> push_data_callback = nullptr;

			// Cancel request buf
			std::map<AString, CancelRequest> cancel_request_buf;// local_orderid:req

			// Callback for processing push order data
			FnMut<void(const CancelRequest&)> _cancel_order = nullptr;
		};
	}
}
