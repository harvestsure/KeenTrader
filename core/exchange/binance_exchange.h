#pragma once

#include <api/RestClient.h>
#include <api/WebsocketClient.h>
#include <engine/exchange.h>

using namespace Keen::api;
using namespace Keen::engine;

/*
* Binance Crypto Exchange::
*/

namespace Keen
{
	namespace engine {
		class EventEmitter;
	}

	namespace exchange
	{
		using EventEmitter = engine::EventEmitter;

		namespace binance
		{
			class BinanceTradeWebsocketApi;
			class BinanceDataWebsocketApi;
			class BinanceRestApi;

			class KEEN_EXCHANGE_EXPORT BinanceExchange : public BaseExchange
			{
				friend BinanceTradeWebsocketApi;
				friend BinanceDataWebsocketApi;
				friend BinanceRestApi;
				/*
				* Binance connection.
				*/

			public:
				BinanceExchange(EventEmitter* event_emitter);
				~BinanceExchange();

				void connect(const Json& setting);

				void subscribe(const SubscribeRequest& req);

				AString send_order(const OrderRequest& req);

				void query_account();

				void query_position();

				std::list<BarData> query_history(HistoryRequest req);

				void close();

				void process_timer_event(const Event& event);

			protected:
				BinanceTradeWebsocketApi* trade_ws_api;
				BinanceDataWebsocketApi* market_ws_api;
				BinanceRestApi* rest_api;
			};

			class BinanceRestApi : public RestClient
			{
				/*
				* BINANCE REST API
				*/
			public:
				BinanceRestApi(BinanceExchange* exchange);

				Request& sign(Request& request);

				void connect(
					AString key,
					AString secret,
					int session_number,
					AString proxy_host,
					uint16_t proxy_port
				);

				void query_time();

				void query_account();

				void query_order();

				void query_contract();

				int _new_order_id();

				AString send_order(const OrderRequest& req);

				void cancel_order(CancelRequest req);

				void start_user_stream();

				void keep_user_stream();

				void on_query_time(const Json& data);

				void on_query_account(const Json& data);

				void on_query_order(const Json& data);

				void on_query_contract(const Json& data);

				void on_send_order(const Json& data);

				void on_send_order_failed(const Error& error, const Request& request);

				void on_send_order_error(const std::exception& ex, const Request& request);

				void on_cancel_order(const Json& data);

				void on_start_user_stream(const Json& data);

				void on_keep_user_stream(const Json& data);

				std::list<BarData> query_history(HistoryRequest req);

			protected:
				BinanceExchange* exchange;
				AString exchange_name;
				BinanceTradeWebsocketApi* trade_ws_api;

				AString key, secret;

				AString proxy_host;
				uint16_t proxy_port;
				
				AString user_stream_key;
				int keep_alive_count;
				int recv_window;
				int time_offset;
				int order_count;
				cCriticalSection order_count_lock;
				uint64_t connect_time;
			};

			class BinanceTradeWebsocketApi : public WebsocketClient
			{
			public:
				BinanceTradeWebsocketApi(BinanceExchange* exchange);

				void connect(AString url, AString proxy_host, uint16_t proxy_port);

				void on_connected();

				void on_packet(Json packet);

				void on_account(const Json& packet);

				void on_order(const Json& packet);

			protected:
				BinanceExchange* exchange;
				AString exchange_name;
			};

			class BinanceDataWebsocketApi :public WebsocketClient
			{
				////
			public:
				BinanceDataWebsocketApi(BinanceExchange* exchange);

				void connect(AString proxy_host, uint16_t proxy_port);

				void on_connected();

				void subscribe(SubscribeRequest req);

				void on_packet(Json packet);

			protected:
				BinanceExchange* exchange;
				AString exchange_name;

				std::map<AString, TickData> ticks;

				AString proxy_host;
				uint16_t proxy_port;
			};

		}
	}
}

