#include <api/Globals.h>
#include <engine/engine.h>
#include <engine/utility.h>
#include "binance_linear_exchange.h"
#include "algo_hmac.h"
#include "utils.h"

using namespace std::placeholders;

namespace Keen
{
    namespace exchange
    {
        namespace binance
        {
            const AString REAL_USER_HOST = "wss://fstream.binance.com/ws/";
            const AString TESTNET_USER_HOST = "wss://stream.binancefuture.com/ws/";
            const int WEBSOCKET_TIMEOUT = 24 * 60 * 60;
            BinanceLinearExchange::BinanceLinearExchange(EventEmitter* event_emitter)
                : BaseExchange(event_emitter, "BINANCE_LINEAR")
            {
                this->rest_api = new BinanceRestApi(this);
                this->public_api = new BinanceMdApi(this);
                this->private_api = new BinanceTradeApi(this);
            }

            BinanceLinearExchange::~BinanceLinearExchange()
            {
                SAFE_RELEASE(rest_api);
                SAFE_RELEASE(public_api);
                SAFE_RELEASE(private_api);
            }

            void BinanceLinearExchange::connect(const Json& setting)
            {
                this->key = setting.value("API Key", "");
                this->secret = setting.value("API Secret", "");
                this->proxy_host = setting.value("Proxy Host", "");
                this->proxy_port = setting.value("Proxy Port", 0);
                this->server = setting.value("Server", "REAL");

                this->rest_api->connect(
                    this->key,
                    this->secret,
                    this->server,
                    this->proxy_host,
                    this->proxy_port);
            }

            void BinanceLinearExchange::process_timer_event(const Event& event)
            {
                if (this->rest_api)
                    this->rest_api->keep_user_stream();
            }

            void BinanceLinearExchange::connect_ws_api()
            {
                this->public_api->connect(
                    this->server,
                    true, // Assuming kline_stream is enabled
                    this->proxy_host,
                    this->proxy_port);

                this->private_api->connect(
                    this->key,
                    this->secret,
                    this->server,
                    this->proxy_host,
                    this->proxy_port);
            }

            void BinanceLinearExchange::subscribe(const SubscribeRequest& req)
            {
                this->public_api->subscribe(req);
            }

            AString BinanceLinearExchange::send_order(const OrderRequest& req)
            {
                return this->private_api->send_order(req);
            }

            void BinanceLinearExchange::cancel_order(const CancelRequest& req)
            {
                this->private_api->cancel_order(req);
            }

            void BinanceLinearExchange::query_account()
            {
                // Not required as Binance provides websocket updates
            }

            void BinanceLinearExchange::query_position()
            {
                // Not required as Binance provides websocket updates
            }

            std::list<BarData> BinanceLinearExchange::query_history(const HistoryRequest& req)
            {
                return this->rest_api->query_history(req);
            }

            void BinanceLinearExchange::close()
            {
                this->rest_api->stop();
                this->public_api->stop();
                this->private_api->stop();
            }

            void BinanceLinearExchange::on_order(const OrderData& order)
            {
                this->orders[order.orderid] = order;
                BaseExchange::on_order(order);
            }

            std::optional<OrderData> BinanceLinearExchange::get_order(const AString& orderid)
            {
                if (this->orders.count(orderid))
                {
                    return this->orders[orderid];
                }
                else
                {
                    return std::nullopt;
                }
            }

            void BinanceLinearExchange::on_contract(const ContractData& contract)
            {
                this->symbol_contract_map[contract.symbol] = contract;
                this->name_contract_map[contract.name] = contract;

                BaseExchange::on_contract(contract);
            }

            std::optional<ContractData> BinanceLinearExchange::get_contract_by_symbol(const AString& symbol)
            {
                return GetWithNone(this->symbol_contract_map, symbol);
            }

            std::optional<ContractData> BinanceLinearExchange::get_contract_by_name(const AString& name)
            {
                return GetWithNone(this->name_contract_map, name);
            }

            BinanceRestApi::BinanceRestApi(BinanceLinearExchange* exchange)
                : RestClient()
            {
                this->exchange = exchange;
                this->gateway_name = exchange->get_exchange_name();
            }

            Request& BinanceRestApi::sign(Request& request)
            {
                // Implement signing logic here
                return request;
            }

            void BinanceRestApi::connect(
                AString key,
                AString secret,
                AString server,
                AString proxy_host,
                uint16_t proxy_port)
            {
                this->key = key;
                this->secret = secret;

                this->init(server, proxy_host, proxy_port);
                this->start();
                this->exchange->write_log("REST API started");
            }

            std::list<BarData> BinanceRestApi::query_history(const HistoryRequest& req)
            {
                // Implement history query logic here
                return {};
            }

            void BinanceRestApi::start()
            {
                RestClient::start();
            }

            void BinanceRestApi::stop()
            {
                RestClient::stop();
            }

            void BinanceRestApi::query_account()
            {
                Request request{
                    .method = "GET",
                    .path = "/fapi/v3/account",
                    .callback = std::bind(&BinanceRestApi::on_query_account, this, _1, _2)
                };
                this->request(request);
            }

            void BinanceRestApi::query_position()
            {
                Request request{
                    .method = "GET",
                    .path = "/fapi/v3/positionRisk",
                    .callback = std::bind(&BinanceRestApi::on_query_position, this, _1, _2)
                };
                this->request(request);
            }

            void BinanceRestApi::start_user_stream()
            {
                Request request{
                    .method = "POST",
                    .path = "/fapi/v1/listenKey",
                    .callback = std::bind(&BinanceRestApi::on_start_user_stream, this, _1, _2)
                };
                this->request(request);
            }

            void BinanceRestApi::keep_user_stream()
            {
                if (this->user_stream_key.empty())
                    return;

                this->keep_alive_count += 1;
                if (this->keep_alive_count < 600)
                    return;
                this->keep_alive_count = 0;

                Params params = {{"listenKey", this->user_stream_key}};
                Request request{
                    .method = "PUT",
                    .path = "/fapi/v1/listenKey",
                    .callback = std::bind(&BinanceRestApi::on_keep_user_stream, this, _1, _2),
                    .params = params
                };
                this->request(request);
            }

            void BinanceRestApi::on_query_account(const Json& packet, const Request& request)
            {
                this->exchange->write_log("RestApi on_query_account (stub)");
            }

            void BinanceRestApi::on_query_position(const Json& packet, const Request& request)
            {
                this->exchange->write_log("RestApi on_query_position (stub)");
            }

            void BinanceRestApi::on_start_user_stream(const Json& packet, const Request& request)
            {
                if (packet.contains("listenKey"))
                {
                    this->user_stream_key = packet["listenKey"];
                    this->keep_alive_count = 0;

                    // connect user api
                    AString url = (this->server == "REAL") ? REAL_USER_HOST + this->user_stream_key : TESTNET_USER_HOST + this->user_stream_key;
                    if (this->exchange->user_api)
                        this->exchange->user_api->connect(url, this->proxy_host, this->proxy_port);
                }
            }

            void BinanceRestApi::on_keep_user_stream(const Json& packet, const Request& request)
            {
                // success - nothing to do
            }

            void BinanceRestApi::on_keep_user_stream_error(const std::type_info& exception_type, const std::exception& exception_value, const void* tb, const Request& request)
            {
                this->exchange->write_log(Printf("keep_user_stream error: %s", exception_value.what()));
            }

            

            BinanceMdApi::BinanceMdApi(BinanceLinearExchange* exchange)
                : WebsocketClient()
            {
                this->exchange = exchange;
                this->gateway_name = exchange->get_exchange_name();
            }

            void BinanceMdApi::connect(
                AString server,
                bool kline_stream,
                AString proxy_host,
                uint16_t proxy_port)
            {
                    this->kline_stream = kline_stream;
                    this->init(server, proxy_host, proxy_port, 20);
                    this->start();
            }

            void BinanceMdApi::subscribe(const SubscribeRequest& req)
            {
                    // stub: register subscription and prepare tick
                    auto contract = this->exchange->get_contract_by_symbol(req.symbol);
                    if (!contract)
                    {
                        this->exchange->write_log(Printf("Failed to subscribe data, symbol not found: %s", req.symbol.c_str()));
                        return;
                    }
                    this->subscribed[req.symbol] = req;
                    TickData tick;
                    tick.symbol = req.symbol;
                    tick.exchange = req.exchange;
                    tick.name = contract->name;
                    tick.datetime = currentDateTime();
                    tick.exchange_name = this->gateway_name;
                    tick.__post_init__();
                    this->ticks[req.symbol] = tick;
                    this->reqid += 1;
                    // send subscribe packet
            }

                void BinanceMdApi::on_connected()
                {
                    this->exchange->write_log("MD API connected");
                    for (auto &p : this->subscribed)
                        this->subscribe(p.second);
                }

                void BinanceMdApi::on_disconnected()
                {
                    this->exchange->write_log("MD API disconnected");
                }

                void BinanceMdApi::on_packet(const Json& packet)
                {
                    // stub: just log
                    (void)packet;
                }

                void BinanceMdApi::on_error(const std::exception& ex)
                {
                    this->exchange->write_log(Printf("MD API exception: %s", ex.what()));
                }
            BinanceTradeApi::BinanceTradeApi(BinanceLinearExchange* exchange)
                : WebsocketClient()
            {
                this->exchange = exchange;
                this->gateway_name = exchange->get_exchange_name();

                this->proxy_host = "";
                this->proxy_port = 0;
                this->server = "";
                this->order_count = 0;
                this->order_prefix = "";
                this->reqid = 0;
            }

            void BinanceTradeApi::connect(
                AString key,
                AString secret,
                AString server,
                AString proxy_host,
                uint16_t proxy_port)
            {
                this->key = key;
                this->secret = secret;
                this->server = server;
                this->proxy_host = proxy_host;
                this->proxy_port = proxy_port;

                // Implement connection logic for trade API
            }

            AString BinanceTradeApi::send_order(const OrderRequest& req)
            {
                // Implement order sending logic
                (void)req;
                return AString();
            }

            void BinanceTradeApi::cancel_order(const CancelRequest& req)
            {
                (void)req;
            }

            void BinanceTradeApi::sign(Params& params)
            {
                // stub: no-op
                (void)params;
            }

            void BinanceTradeApi::on_connected()
            {
                this->exchange->write_log("Trade API connected");
            }

            void BinanceTradeApi::on_disconnected()
            {
                this->exchange->write_log("Trade API disconnected");
            }

            void BinanceTradeApi::on_packet(const Json& packet)
            {
                int id = packet.value("id", 0);
                auto it = this->reqid_callback_map.find(id);
                if (it != this->reqid_callback_map.end())
                {
                    it->second(packet);
                }
            }

            void BinanceTradeApi::on_send_order(const Json& packet)
            {
                (void)packet;
            }

            void BinanceTradeApi::on_cancel_order(const Json& packet)
            {
                (void)packet;
            }

            void BinanceTradeApi::on_error(const std::exception& ex)
            {
                this->exchange->write_log(Printf("Trade API exception: %s", ex.what()));
            }

            BinanceUserApi::BinanceUserApi(BinanceLinearExchange* exchange)
                : WebsocketClient()
            {
                this->exchange = exchange;
                this->gateway_name = exchange->get_exchange_name();
            }

            void BinanceUserApi::connect(
                AString url,
                AString proxy_host,
                uint16_t proxy_port)
            {
                this->init(url, proxy_host, proxy_port, WEBSOCKET_TIMEOUT);
                this->start();
            }

            void BinanceUserApi::on_connected()
            {
                this->exchange->write_log("User API connected");
            }

            void BinanceUserApi::on_disconnected()
            {
                this->exchange->write_log("User API disconnected");
                this->exchange->rest_api->start_user_stream();
            }

            void BinanceUserApi::on_packet(const Json& packet)
            {
                if (!packet.contains("e"))
                    return;
                AString evt = packet["e"];
                if (evt == "ACCOUNT_UPDATE")
                    this->on_account(packet);
                else if (evt == "ORDER_TRADE_UPDATE")
                    this->on_order(packet);
                else if (evt == "listenKeyExpired")
                    this->on_listen_key_expired();
            }

            void BinanceUserApi::on_error(const std::exception& ex)
            {
                this->exchange->write_log(Printf("User API exception: %s", ex.what()));
            }

            void BinanceUserApi::on_listen_key_expired()
            {
                this->exchange->write_log("Listen key expired");
                this->stop();
            }

            void BinanceUserApi::on_account(const Json& packet)
            {
                (void)packet;
            }

            void BinanceUserApi::on_order(const Json& packet)
            {
                (void)packet;
            }

            void BinanceUserApi::on_disconnected(int status_code, const AString& msg)
            {
                this->exchange->write_log(Printf("User API disconnected, code: %d, msg: %s", status_code, msg.c_str()));
                this->exchange->rest_api->start_user_stream();
            }
        }
    }
}