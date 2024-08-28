#pragma once

#include <engine/object.h>

namespace Keen
{
	namespace engine
	{
		class Event;
		class EventEmitter;
		class BaseApp;
		class BaseEngine;
		class BaseExchange;
		class OffsetConverter;

		class KEEN_ENGINE_EXPORT TradeEngine
		{
		public:

			TradeEngine(EventEmitter* event_emitter);
			virtual ~TradeEngine();

			BaseEngine* add_engine(BaseEngine* engine);

			template<class T = BaseExchange>
			T* add_exchange()
			{
				T* exchange = new T(this->event_emitter);
				this->exchanges[exchange->exchange_name] = exchange;

				if (!this->exchanges.count(exchange->exchange_name))
				{
					this->exchanges.insert(std::pair(exchange->exchange_name, exchange));
				}

				return exchange;
			}

			template<class T = BaseApp>
			BaseEngine* add_app() {

				T* app = new T();
				this->apps[app->appName()] = app;

				auto engine = this->add_engine(app->engine_class(this, this->event_emitter));
				return engine;
			}

			void init_engines();

			void write_log(AString msg, AString source = "");

			BaseExchange* get_exchange(AString exchange_name);

			BaseEngine* get_engine(AString engine_name);

			Json get_default_setting(AString exchange_name);

			AStringList get_all_exchange_names();

			std::list<BaseApp*> get_all_apps();

			std::list<Exchange> get_all_exchanges();

			void connect(const Json& setting, AString exchange_name);

			void subscribe(const SubscribeRequest& req, AString exchange_name);

			void cancel_order(const CancelRequest& req, AString exchange_name);

			AString send_quote(const QuoteRequest& req, AString exchange_name);

			void cancel_quote(const CancelRequest& req, AString exchange_name);

			std::list<BarData> query_history(const HistoryRequest& req, AString exchange_name);

			void close();

		public:
			FnMut<AString(const OrderRequest&, AString)> send_order;

			FnMut<std::optional<TickData>(AString)> get_tick;
			FnMut<std::optional<OrderData>(AString)> get_order;
			FnMut<std::optional<TradeData>(AString)> get_trade;
			FnMut<std::optional<PositionData>(AString)> get_position;
			FnMut<std::optional<AccountData>(AString)> get_account;
			FnMut<std::optional<ContractData>(AString)> get_contract;
			FnMut<std::optional<QuoteData>(AString)> get_quote;
			FnMut <std::list<TickData>()> get_all_ticks;
			FnMut <std::list<OrderData>()> get_all_orders;
			FnMut <std::list<TradeData>()> get_all_trades;
			FnMut <std::list<PositionData>()> get_all_positions;
			FnMut <std::list<AccountData>()> get_all_accounts;
			FnMut <std::list<ContractData>()> get_all_contracts;
			FnMut <std::list<QuoteData>()> get_all_quotes;
			FnMut <std::list<OrderData>(AString)> get_all_active_orders;
			FnMut <std::list<QuoteData>(AString)> get_all_active_quotes;

			FnMut <void(const OrderRequest&, const AString&, const AString&)> update_order_request;
			FnMut <std::list<OrderRequest>(const OrderRequest&, const AString&, bool, bool)> convert_order_request;
			FnMut <OffsetConverter* (const AString&)> get_converter;

			FnMut<void(AString, AString, AString)> send_email;

		protected:
			EventEmitter* event_emitter;
			std::map<AString, BaseExchange*> exchanges;
			std::map<AString, BaseEngine*> engines;
			std::map<AString, BaseApp*> apps;
		};


		class KEEN_ENGINE_EXPORT BaseEngine
		{
		public:
			BaseEngine(
				TradeEngine* trade_engine,
				EventEmitter* event_emitter,
				AString engine_name);

			virtual ~BaseEngine();

			virtual void close();

		public:
			TradeEngine* trade_engine;
			EventEmitter* event_emitter;
			AString engine_name;
		};

		class KEEN_ENGINE_EXPORT LogEngine : public BaseEngine
		{
		public:

			LogEngine(TradeEngine* trade_engine, EventEmitter* event_emitter);
			~LogEngine();

			void add_console_handler();

			void add_file_handler();

			void register_event();

			void process_log_event(const Event& event);
		};


		class OmsEngine : public BaseEngine
		{
		public:
			OmsEngine(TradeEngine* trade_engine, EventEmitter* event_emitter);

			virtual ~OmsEngine();

			void add_function();

			void register_event();

			void process_tick_event(const Event& event);

			void process_order_event(const Event& event);

			void process_trade_event(const Event& event);

			void process_position_event(const Event& event);

			void process_account_event(const Event& event);

			void process_contract_event(const Event& event);

			void process_quote_event(const Event& event);

			std::optional<TickData> get_tick(AString kt_symbol);

			std::optional<OrderData> get_order(AString kt_orderid);

			std::optional<TradeData> get_trade(AString kt_tradeid);

			std::optional<PositionData> get_position(AString kt_positionid);

			std::optional<AccountData> get_account(AString kt_accountid);

			std::optional<ContractData> get_contract(AString kt_symbol);

			std::optional<QuoteData> get_quote(AString kt_quoteid);

			std::list<TickData> get_all_ticks();

			std::list<OrderData> get_all_orders();

			std::list<TradeData> get_all_trades();

			std::list<PositionData> get_all_positions();

			std::list<AccountData> get_all_accounts();

			std::list<ContractData> get_all_contracts();

			std::list<QuoteData> get_all_quotes();

			std::list<OrderData> get_all_active_orders(AString kt_symbol = "");

			std::list<QuoteData> get_all_active_quotes(AString kt_symbol = "");

			void update_order_request(const OrderRequest& req, const AString& kt_orderid, const AString& exchange_name);

			std::list<OrderRequest> convert_order_request(const OrderRequest& req,
				const AString& exchange_name, bool lock, bool net = false);

			OffsetConverter* get_converter(const AString& exchange_name);

		protected:
			std::map<AString, TickData> ticks;
			std::map<AString, OrderData> orders;
			std::map<AString, TradeData> trades;
			std::map<AString, PositionData> positions;
			std::map<AString, AccountData> accounts;
			std::map<AString, ContractData> contracts;
			std::map<AString, QuoteData> quotes;

			std::map<AString, OrderData> active_orders;
			std::map<AString, QuoteData> active_quotes;

			std::map<AString, OffsetConverter*> offset_converters;
		};



		class EmailEngine : public BaseEngine
		{
		public:
			EmailEngine(TradeEngine* trade_engine, EventEmitter* event_emitter);

			void send_email(AString subject, AString content, AString receiver = "");

			void run();

			void start();

			void close();

		protected:
			bool _active;
			std::queue<EmailMessage> _queue;
			cCriticalSection	m_CS;
			cEvent				m_QueueNonempty;
			std::thread _thread;
		};

	}
}
