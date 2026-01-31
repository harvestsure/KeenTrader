#pragma once

#include <api/RestClient.h>
#include <api/WebsocketClient.h>
#include <engine/exchange.h>
#include <map>
#include <list>
#include <string>
#include <optional>

using namespace Keen::api;
using namespace Keen::engine;

namespace Keen
{
    namespace exchange
    {
        namespace binance
        {
            class BinanceRestApi;
            class BinanceMdApi;
            class BinanceTradeApi;
            class BinanceUserApi;

            class KEEN_EXCHANGE_EXPORT BinanceLinearExchange : public BaseExchange
            {
            public:
                BinanceLinearExchange(EventEmitter* event_emitter);
                virtual ~BinanceLinearExchange();

                void connect(const Json& setting) override;
                void connect_ws_api();
                void subscribe(const SubscribeRequest& req) override;
                AString send_order(const OrderRequest& req) override;
                void cancel_order(const CancelRequest& req) override;
                void query_account() override;
                void query_position() override;
                std::list<BarData> query_history(const HistoryRequest& req) override;
                void process_timer_event(const Event& event);
                void close() override;

                void on_order(const OrderData& order) override;
                std::optional<OrderData> get_order(const AString& orderid);
                void on_contract(const ContractData& contract) override;
                std::optional<ContractData> get_contract_by_symbol(const AString& symbol);
                std::optional<ContractData> get_contract_by_name(const AString& name);

            protected:
                std::map<AString, OrderData> orders;
                std::map<AString, ContractData> symbol_contract_map;
                std::map<AString, ContractData> name_contract_map;

                BinanceRestApi* rest_api = nullptr;
                BinanceMdApi* md_api = nullptr;
                BinanceTradeApi* trade_api = nullptr;
                BinanceUserApi* user_api = nullptr;

            private:
                AString key;
                    friend class BinanceRestApi;
                    friend class BinanceMdApi;
                    friend class BinanceTradeApi;
                    friend class BinanceUserApi;
                AString secret;
                AString proxy_host;
                uint16_t proxy_port;
                AString server;
            };

            class BinanceRestApi : public RestClient
            {
            public:
                BinanceRestApi(BinanceLinearExchange* exchange);

                Request& sign(Request& request) override;
                void connect(
                    AString key,
                    AString secret,
                    AString server,
                    AString proxy_host,
                    uint16_t proxy_port
                );
                void start();
                void stop();
                void query_order();
                void query_account();
                void query_position();
                void query_time();
                void query_contract();
                void start_user_stream();
                void keep_user_stream();
                void on_query_order(const Json& packet, const Request& request);
                void on_query_account(const Json& packet, const Request& request);
                void on_query_time(const Json& packet, const Request& request);
                void on_query_position(const Json& packet, const Request& request);
                void on_query_contract(const Json& packet, const Request& request);
                void on_start_user_stream(const Json& packet, const Request& request);
                void on_keep_user_stream(const Json& packet, const Request& request);
                void on_keep_user_stream_error(const std::type_info& exception_type, const std::exception& exception_value, const void* tb, const Request& request);
                std::list<BarData> query_history(const HistoryRequest& req);

            protected:
                BinanceLinearExchange* exchange;
                AString exchange_name;
                AString key;
                AString secret;
                uint64_t connect_time;
                // user stream
                AString user_stream_key;
                int keep_alive_count = 0;
                int time_offset = 0;

                int order_count = 1000000;
                AString order_prefix;
                // network helpers
                AString server;
                AString proxy_host;
                uint16_t proxy_port = 0;
            };

            class BinanceMdApi : public WebsocketClient
            {
            public:
                BinanceMdApi(BinanceLinearExchange* exchange);
                void connect(
                    AString server,
                    bool kline_stream,
                    AString proxy_host,
                    uint16_t proxy_port
                );
                void subscribe(const SubscribeRequest& req);
                void on_connected() override;
                void on_disconnected() override;
                void on_packet(const Json& packet) override;
                void on_error(const std::exception& ex) override;

            protected:
                BinanceLinearExchange* exchange;
                AString exchange_name;
                // connection info
                AString server;
                AString proxy_host;
                uint16_t proxy_port = 0;
                std::map<AString, SubscribeRequest> subscribed;
                std::map<AString, TickData> ticks;
                int reqid = 0;
                bool kline_stream = false;
            };

            class BinanceTradeApi : public WebsocketClient
            {
            public:
                BinanceTradeApi(BinanceLinearExchange* exchange);
                void connect(
                    AString key,
                    AString secret,
                    AString server,
                    AString proxy_host,
                    uint16_t proxy_port
                );
                AString send_order(const OrderRequest& req);
                void cancel_order(const CancelRequest& req);
                void sign(Params& params);
                void on_connected() override;
                void on_disconnected() override;
                void on_packet(const Json& packet) override;
                void on_send_order(const Json& packet);
                void on_cancel_order(const Json& packet);
                void on_error(const std::exception& ex) override;

            protected:
                BinanceLinearExchange* exchange;
                AString exchange_name;
                AString key;
                AString secret;
                int reqid;
                // connection / order helpers
                AString proxy_host;
                uint16_t proxy_port = 0;
                AString server;
                int order_count = 0;
                AString order_prefix;

                std::map<int, std::function<void(const Json&)>> reqid_callback_map;
                std::map<int, OrderData> reqid_order_map;
            };

            class BinanceUserApi : public WebsocketClient
            {
            public:
                BinanceUserApi(BinanceLinearExchange* exchange);
                void connect(
                    AString url,
                    AString proxy_host,
                    uint16_t proxy_port
                );
                void on_connected() override;
                void on_disconnected() override;
                void on_packet(const Json& packet) override;
                void on_error(const std::exception& ex) override;
                void on_listen_key_expired();
                void on_account(const Json& packet);
                void on_order(const Json& packet);
                void on_disconnected(int status_code, const AString& msg);

            protected:
                BinanceLinearExchange* exchange;
                AString exchange_name;
            };
        }
    }
}