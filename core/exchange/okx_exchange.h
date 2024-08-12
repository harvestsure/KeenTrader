#pragma once

#include <api/RestClient.h>
#include <api/WebsocketClient.h>
#include <engine/exchange.h>

using namespace Keen::api;
using namespace Keen::engine;

namespace Keen
{
	namespace engine {
		class EventEmitter;
	}

	namespace exchange
	{
		using EventEmitter = engine::EventEmitter;

		namespace okx
		{
			class OkxRestApi;
			class OkxWebsocketPublicApi;
			class OkxWebsocketPrivateApi;

			class KEEN_EXCHANGE_EXPORT OkxExchange : public BaseExchange
			{
				friend OkxRestApi;
				friend OkxWebsocketPublicApi;
				friend OkxWebsocketPrivateApi;

			public:
				OkxExchange(EventEmitter* event_emitter);
				virtual ~OkxExchange();

				void connect(const Json& setting);

				void subscribe(const SubscribeRequest& req);

				AString send_order(const OrderRequest& req);

				void cancel_order(const CancelRequest& req);

				void query_account();

				void query_position();

				std::list<BarData> query_history(const HistoryRequest& req);

				void close();

				void on_order(const OrderData& order);

				std::optional<OrderData> get_order(AString orderid);

			protected:
				Json voidault_setting;

				std::map<AString, OrderData> orders;

				OkxRestApi* rest_api = nullptr;
				OkxWebsocketPublicApi* ws_public_api = nullptr;
				OkxWebsocketPrivateApi* ws_private_api = nullptr;
			};

			class KEEN_EXCHANGE_EXPORT OkxRestApi : public RestClient
			{
			public:
				OkxRestApi(OkxExchange* exchange);

				Request& sign(Request& request) override;

				void connect(
					AString key,
					AString secret,
					AString passphrase,
					AString proxy_host,
					uint16_t proxy_port,
					AString server
				);

				void query_order();
				void query_time();
				void query_contract();

				void on_query_order(const Json& packet, const Request& request);
				void on_query_time(const Json& packet, const Request& request);
				void on_instrument(const Json& packet, const Request& request);

				void on_error(const std::exception& ex, const Request& request) override;

				std::list<BarData> query_history(const HistoryRequest& req);

			protected:
				OkxExchange* exchange;
				AString exchange_name;

				AString key;
				AString secret;
				AString passphrase;

				bool simulated;

				uint64_t connect_time;
			};

			class KEEN_EXCHANGE_EXPORT OkxWebsocketPublicApi : public WebsocketClient
			{
			public:

				OkxWebsocketPublicApi(OkxExchange* exchange);

				void connect(
					AString proxy_host,
					uint16_t proxy_port,
					AString server
				);

				void subscribe(const SubscribeRequest& req);

				void on_connected() override;

				void on_disconnected() override;

				void on_packet(const Json& packet) override;

				void on_error(const std::exception& ex) override;

				void on_ticker(const Json& data);

				void on_depth(const Json& data);

			protected:
				OkxExchange* exchange;
				AString exchange_name;

				std::map<AString, SubscribeRequest> subscribed;
				std::map<AString, TickData> ticks;
				std::map<AString, FnMut<void(const Json&)>> callbacks;
			};

			class KEEN_EXCHANGE_EXPORT OkxWebsocketPrivateApi : public WebsocketClient
			{
			public:
				OkxWebsocketPrivateApi(OkxExchange* exchange);

				void connect(
					AString key,
					AString secret,
					AString passphrase,
					AString proxy_host,
					uint16_t proxy_port,
					AString server
				);

				void on_connected() override;

				void on_disconnected() override;

				void on_packet(const Json& packet) override;

				void on_error(const std::exception& ex) override;

				void on_api_error(const Json& packet);

				void on_login(const Json& packet);

				void on_order(const Json& packet);

				void on_account(const Json& packet);

				void on_position(const Json& packet);

				void on_send_order(const Json& packet);

				void on_cancel_order(const Json& packet);

				void login();

				void subscribe_topic();

				AString send_order(const OrderRequest& req);

				void cancel_order(const CancelRequest& req);

			protected:
				OkxExchange* exchange;
				AString exchange_name;

				AString key;
				AString secret;
				AString passphrase;

				int reqid;
				int order_count;
				uint64_t connect_time;

				std::map<AString, OrderData> reqid_order_map;
				std::map<AString, FnMut<void(const Json&)>> callbacks;

			};
		}
	}
}

