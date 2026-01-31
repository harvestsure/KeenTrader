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
            // Real server hosts
            const AString REAL_REST_HOST = "https://fapi.binance.com";
            const AString REAL_TRADE_HOST = "wss://ws-fapi.binance.com/ws-fapi/v1";
            const AString REAL_USER_HOST = "wss://fstream.binance.com/ws/";
            const AString REAL_DATA_HOST = "wss://fstream.binance.com/stream";

            // Testnet server hosts
            const AString TESTNET_REST_HOST = "https://demo-fapi.binance.com";
            const AString TESTNET_TRADE_HOST = "wss://testnet.binancefuture.com/ws-fapi/v1";
            const AString TESTNET_USER_HOST = "wss://fstream.binancefuture.com/ws/";
            const AString TESTNET_DATA_HOST = "wss://fstream.binancefuture.com/stream";

            const int WEBSOCKET_TIMEOUT = 24 * 60 * 60;

            static std::map<AString, Product> PRODUCT_BINANCE2KT = {
                {"PERPETUAL", Product::SWAP},
                {"PERPETUAL_DELIVERING", Product::SWAP},
                {"CURRENT_MONTH", Product::FUTURES},
                {"NEXT_MONTH", Product::FUTURES},
                {"CURRENT_QUARTER", Product::FUTURES},
                {"NEXT_QUARTER", Product::FUTURES}
            };

            static std::map<AString, Status> STATUS_BINANCE2KT = {
                {"NEW", Status::NOTTRADED},
                {"PARTIALLY_FILLED", Status::PARTTRADED},
                {"FILLED", Status::ALLTRADED},
                {"CANCELED", Status::CANCELLED},
                {"EXPIRED", Status::CANCELLED},
                {"REJECTED", Status::REJECTED}
            };

            static std::map<AString, Direction> DIRECTION_BINANCE2VT = {
                {"BUY", Direction::LONG},
                {"SELL", Direction::SHORT}
            };

            static std::map<Direction, AString> DIRECTION_VT2BINANCE = {
                {Direction::LONG, "BUY"},
                {Direction::SHORT, "SELL"}
            };

            static std::map<OrderType, AString> ORDERTYPE_VT2BINANCE = {
                {OrderType::MARKET, "MARKET"},
                {OrderType::STOP, "STOP_MARKET"},
                {OrderType::LIMIT, "LIMIT"},
                {OrderType::FAK, "LIMIT"},
                {OrderType::FOK, "LIMIT"}
            };

            static std::map<AString, OrderType> ORDERTYPE_BINANCE2VT = {
                {"MARKET", OrderType::MARKET},
                {"STOP_MARKET", OrderType::STOP},
                {"LIMIT", OrderType::LIMIT}
            };

            BinanceLinearExchange::BinanceLinearExchange(EventEmitter* event_emitter)
                : BaseExchange(event_emitter, "BINANCE")
                , proxy_port(0)
            {
                this->rest_api = new BinanceRestApi(this);
                this->md_api = new BinanceMdApi(this);
                this->trade_api = new BinanceTradeApi(this);
                this->user_api = new BinanceUserApi(this);
            }

            BinanceLinearExchange::~BinanceLinearExchange()
            {
                SAFE_RELEASE(rest_api);
                SAFE_RELEASE(md_api);
                SAFE_RELEASE(trade_api);
                SAFE_RELEASE(user_api);
            }

            void BinanceLinearExchange::connect(const Json& setting)
            {
                this->key = setting.value("api_key", "");
                this->secret = setting.value("secret_key", "");
                this->proxy_host = setting.value("proxy_host", "");
                this->proxy_port = setting.value("proxy_port", 0);
                this->server = setting.value("server", "");

                this->rest_api->connect(
                    this->key,
                    this->secret,
                    this->server,
                    this->proxy_host,
                    this->proxy_port);

                this->connect_ws_api();
            }

            void BinanceLinearExchange::process_timer_event(const Event& event)
            {
                if (this->rest_api)
                    this->rest_api->keep_user_stream();
            }

            void BinanceLinearExchange::connect_ws_api()
            {
                this->md_api->connect(
                    this->server,
                    true, // Assuming kline_stream is enabled
                    this->proxy_host,
                    this->proxy_port);

                this->trade_api->connect(
                    this->key,
                    this->secret,
                    this->server,
                    this->proxy_host,
                    this->proxy_port);
            }

            void BinanceLinearExchange::subscribe(const SubscribeRequest& req)
            {
                this->md_api->subscribe(req);
            }

            AString BinanceLinearExchange::send_order(const OrderRequest& req)
            {
                return this->trade_api->send_order(req);
            }

            void BinanceLinearExchange::cancel_order(const CancelRequest& req)
            {
                this->trade_api->cancel_order(req);
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
                this->user_api->stop();
                this->md_api->stop();
                this->trade_api->stop();
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
                , connect_time(0)
            {
                this->exchange = exchange;
                this->exchange_name = exchange->get_exchange_name();
            }

            Request& BinanceRestApi::sign(Request& request)
            {
                // Build query string from params
                if (request.params.empty())
                {
                    request.params = Params();
                }

                // get timestamp in ms
                long long timestamp = (long long)(time(nullptr) * 1000);

                // adjust with time offset if set
                if (this->time_offset > 0)
                    timestamp -= std::llabs(this->time_offset);
                else if (this->time_offset < 0)
                    timestamp += std::llabs(this->time_offset);

                request.params["timestamp"] = std::to_string(timestamp);

                // create ordered query string
                std::vector<std::pair<std::string, std::string>> items;
                for (auto &p : request.params)
                    items.emplace_back(p.first, p.second);
                std::sort(items.begin(), items.end(), [](auto &a, auto &b){ return a.first < b.first; });

                std::string query;
                for (size_t i = 0; i < items.size(); ++i)
                {
                    if (i) query += "&";
                    query += items[i].first + "=" + std::string(URLEncode(items[i].second));
                }

                // signature HMAC-SHA256 using HmacEncode + hex_str
                unsigned char* mac = nullptr;
                unsigned int mac_len = 0;
                int rc = HmacEncode("sha256", this->secret.c_str(), (unsigned int)this->secret.length(), query.c_str(), (unsigned int)query.length(), mac, mac_len);
                std::string signature;
                if (rc == 0 && mac && mac_len > 0)
                {
                    std::vector<unsigned char> outbuf(mac_len * 2 + 1);
                    hex_str(mac, mac_len, outbuf.data());
                    signature.assign((char*)outbuf.data());
                    free(mac);
                }

                if (!query.empty())
                    query += "&signature=" + signature;
                else
                    query = "signature=" + signature;

                request.path = request.path + "?" + query;
                request.params.clear();
                request.data = AString();

                request.headers = {
                    {"Content-Type", "application/x-www-form-urlencoded"},
                    {"Accept", "application/json"},
                    {"X-MBX-APIKEY", this->key},
                    {"Connection", "close"}
                };

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
                this->proxy_port = proxy_port;
                this->proxy_host = proxy_host;
                this->server = server;

                // order prefix
                this->order_prefix = DateTimeToString(currentDateTime(), "%y%m%d%H%M%S");

                // choose REST host
                if (this->server == "REAL")
                {
                    this->init(REAL_REST_HOST, proxy_host, proxy_port);
                }
                else
                {
                    this->init(TESTNET_REST_HOST, proxy_host, proxy_port);
                }

                this->start();
                this->exchange->write_log("REST API started");

                // query server time to compute offset
                this->query_time();
            }

            void BinanceRestApi::query_time()
            {
                Request request{
                    .method = "GET",
                    .path = "/fapi/v1/time",
                    .callback = std::bind(&BinanceRestApi::on_query_time, this, _1, _2)
                };
                this->request(request);
            }

            void BinanceRestApi::query_contract()
            {
                Request request{
                    .method = "GET",
                    .path = "/fapi/v1/exchangeInfo",
                    .callback = std::bind(&BinanceRestApi::on_query_contract, this, _1, _2)
                };
                this->request(request);
            }

            void BinanceRestApi::query_order()
            {
                Request request{
                    .method = "GET",
                    .path = "/fapi/v1/openOrders",
                    .callback = std::bind(&BinanceRestApi::on_query_order, this, _1, _2)
                };
                this->request(request);
            }

            std::list<BarData> BinanceRestApi::query_history(const HistoryRequest& req)
            {
                auto opt_contract = this->exchange->get_contract_by_symbol(req.symbol);
                if (!opt_contract)
                {
                    this->exchange->write_log(Printf("Query kline history failed, symbol not found: %s", req.symbol.c_str()));
                    return {};
                }

                ContractData contract = *opt_contract;

                std::list<BarData> history;
                int limit = 1500;

                // start time in milliseconds
                long long start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(req.start.time_since_epoch()).count();

                while (true)
                {
                    AString path = "/fapi/v1/klines";

                    Params params = {
                        {"symbol", contract.name},
                        {"interval", (req.interval == Interval::MINUTE) ? "1m" : (req.interval == Interval::HOUR) ? "1h" : "1d"},
                        {"limit", std::to_string(limit)},
                        {"startTime", std::to_string(start_ms)}
                    };

                    // add end time if provided (non-zero)
                    if (req.end.time_since_epoch().count() != 0)
                    {
                        long long end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(req.end.time_since_epoch()).count();
                        params["endTime"] = std::to_string(end_ms);
                    }

                    Response resp = this->request("GET", path, params);

                    if (resp.status / 100 != 2)
                    {
                        AString msg = Printf("Query kline history failed, status code: %d, information: %s", resp.status, resp.body.c_str());
                        this->exchange->write_log(msg);
                        break;
                    }

                    try {
                        Json data = Json::parse(resp.body);

                        if (!data.is_array() || data.empty())
                        {
                            AString msg = Printf("No kline history data is received, symbol: %s", req.symbol.c_str());
                            this->exchange->write_log(msg);
                            break;
                        }

                        std::vector<BarData> buf;

                        for (const Json& row : data)
                        {
                            if (!row.is_array() || row.size() < 6)
                                continue;

                            long long ts = 0;
                            if (row[0].is_number())
                                ts = row[0].get<long long>();
                            else
                                ts = std::atoll(AString(row[0]).c_str());

                            BarData bar;
                            bar.symbol = req.symbol;
                            bar.exchange = req.exchange;
                            bar.datetime = DateTimeFromTimestamp(ts);
                            bar.interval = req.interval;
                            bar.volume = JsonToFloat(row[5]);
                            // quote asset volume at index 7 if exists
                            if (row.size() > 7)
                                bar.open_interest = JsonToFloat(row[7]);
                            bar.open_price = JsonToFloat(row[1]);
                            bar.high_price = JsonToFloat(row[2]);
                            bar.low_price = JsonToFloat(row[3]);
                            bar.close_price = JsonToFloat(row[4]);
                            bar.exchange_name = this->exchange_name;
                            bar.__post_init__();

                            buf.push_back(bar);
                        }

                        if (buf.empty())
                            break;

                        DateTime begin_dt = buf.front().datetime;
                        DateTime end_dt = buf.back().datetime;

                        for (auto &b : buf)
                            history.push_back(b);

                        AString msg = Printf("Query kline history finished, %s - %s, %s - %s",
                            req.symbol.c_str(), interval_to_str(req.interval).c_str(),
                            DateTimeToString(begin_dt).c_str(), DateTimeToString(end_dt).c_str());
                        this->exchange->write_log(msg);

                        // Break if latest data received
                        if ((int)data.size() < limit || (req.end.time_since_epoch().count() != 0 && end_dt >= req.end))
                            break;

                        // move start to next bar
                        long long add_ms = 60000; // default 1 minute
                        if (req.interval == Interval::HOUR)
                            add_ms = 60LL * 60LL * 1000LL;
                        else if (req.interval == Interval::DAILY)
                            add_ms = 24LL * 60LL * 60LL * 1000LL;

                        start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_dt.time_since_epoch()).count() + add_ms;
                    }
                    catch (const std::exception& e)
                    {
                        this->exchange->write_log(Printf("JSON parsing failed: %s", e.what()));
                        break;
                    }
                }

                // sort by datetime
                history.sort([](const BarData& a, const BarData& b) { return a.datetime < b.datetime; });

                // remove last unclosed kline
                if (!history.empty())
                    history.pop_back();

                return history;
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
                    .params = params,
                    .callback = std::bind(&BinanceRestApi::on_keep_user_stream, this, _1, _2)
                };
                this->request(request);
            }

            void BinanceRestApi::on_query_account(const Json& packet, const Request& request)
            {
                // parse assets
                if (packet.contains("assets") && packet["assets"].is_array())
                {
                    for (auto &asset : packet["assets"])
                    {
                        AccountData account;
                        account.accountid = asset.value("asset", "");
                        account.balance = JsonToFloat(asset["walletBalance"]);
                        account.frozen = JsonToFloat(asset["maintMargin"]);
                        account.exchange_name = this->exchange_name;
                        account.__post_init__();
                        this->exchange->on_account(account);
                    }
                }
                this->exchange->write_log("Account data received");
            }

            void BinanceRestApi::on_query_position(const Json& packet, const Request& request)
            {
                if (packet.is_array())
                {
                    for (auto &d : packet)
                    {
                        AString name = d.value("symbol", "");
                        auto opt_contract = this->exchange->get_contract_by_name(name);
                        if (!opt_contract)
                            continue;

                        ContractData contract = *opt_contract;
                        PositionData position;
                        position.symbol = contract.symbol;
                        position.exchange = Exchange::BINANCE;
                        position.direction = Direction::NET;
                        position.volume = std::stof(d.value("positionAmt", "0"));
                        position.price = d.value("entryPrice", 0.0);
                        position.pnl = d.value("unRealizedProfit", 0.0);
                        position.exchange_name = this->exchange_name;
                        position.__post_init__();

                        this->exchange->on_position(position);
                    }
                }
                this->exchange->write_log("Position data received");
            }

            void BinanceRestApi::on_query_time(const Json& packet, const Request& request)
            {
                long long local_time = (long long)time(nullptr) * 1000;
                long long server_time = packet.value("serverTime", 0LL);
                this->time_offset = (int)(local_time - server_time);
                this->exchange->write_log(Printf("Server time updated, local offset: %dms", this->time_offset));

                this->query_contract();
            }

            void BinanceRestApi::on_query_contract(const Json& packet, const Request& request)
            {
                if (!packet.contains("symbols"))
                    return;

                for (auto &d : packet["symbols"])
                {
                    float pricetick = 1.0f;
                    float min_volume = 1.0f;
                    float max_volume = 1.0f;

                    if (d.contains("filters") && d["filters"].is_array())
                    {
                        for (auto &f : d["filters"])
                        {
                            AString t = f.value("filterType", "");
                            if (t == "PRICE_FILTER")
                                pricetick = std::stof(f.value("tickSize", "1"));
                            else if (t == "LOT_SIZE")
                            {
                                min_volume = std::stof(f.value("minQty", "1"));
                                max_volume = std::stof(f.value("maxQty", "0"));
                            }
                        }
                    }

                    AString contractType = d.value("contractType", "PERPETUAL");
                    Product product = Product::SWAP;
                    if (PRODUCT_BINANCE2KT.count(contractType))
                        product = PRODUCT_BINANCE2KT[contractType];

                    AString name = d.value("symbol", "");
                    AString symbol;
                    if (product == Product::SWAP)
                        symbol = name + "_SWAP";
                    else
                        symbol = name;

                    ContractData contract;
                    contract.symbol = symbol;
                    contract.exchange = Exchange::BINANCE;
                    contract.name = name;
                    contract.pricetick = pricetick;
                    contract.size = 1;
                    contract.min_volume = min_volume;
                    contract.max_volume = max_volume;
                    contract.product = product;
                    contract.net_position = true;
                    contract.history_data = true;
                    contract.exchange_name = this->exchange_name;
                    contract.stop_supported = true;
                    contract.__post_init__();

                    this->exchange->on_contract(contract);
                }

                this->exchange->write_log("Contract data received");

                if (!this->key.empty() && !this->secret.empty())
                {
                    this->query_order();
                    this->query_account();
                    this->query_position();
                    this->start_user_stream();
                }
            }

            void BinanceRestApi::on_query_order(const Json& packet, const Request& request)
            {
                if (!packet.is_array())
                    return;

                for (auto &d : packet)
                {
                    // basic mapping: construct OrderData with available fields
                    AString type = d.value("type", "");
                    AString tif = d.value("timeInForce", "GTC");

                    AString name = d.value("symbol", "");
                    // try standard swap symbol suffix first (consistent with contract creation)
                    auto opt_contract = this->exchange->get_contract_by_symbol(name + "_SWAP");
                    if (!opt_contract)
                        opt_contract = this->exchange->get_contract_by_name(name);
                    if (!opt_contract)
                        continue;

                    ContractData contract = *opt_contract;

                    OrderData order;
                    // map order type (if available)
                    OrderType order_type = OrderType::LIMIT;
                    auto it_ot = ORDERTYPE_BINANCE2VT.find(type);
                    if (it_ot != ORDERTYPE_BINANCE2VT.end())
                        order_type = it_ot->second;
                    if (order_type == OrderType::LIMIT)
                    {
                        if (tif == "IOC") order_type = OrderType::FAK;
                        else if (tif == "FOK") order_type = OrderType::FOK;
                    }
                    order.orderid = d.value("clientOrderId", "");
                    order.symbol = contract.symbol;
                    order.exchange = Exchange::BINANCE;
                    order.price = d.value("price", 0.0);
                    order.volume = d.value("origQty", 0.0);
                    order.traded = d.value("executedQty", 0.0);
                    order.type = order_type;
                    // map status using lookup table
                    AString status = d.value("status", "");
                    auto it_status = STATUS_BINANCE2KT.find(status);
                    if (it_status != STATUS_BINANCE2KT.end())
                        order.status = it_status->second;
                    else
                        order.status = Status::NOTTRADED;
                    order.datetime = currentDateTime();
                    order.exchange_name = this->exchange_name;
                    order.__post_init__();

                    this->exchange->on_order(order);
                }

                this->exchange->write_log("Order data received");
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
                this->exchange_name = exchange->get_exchange_name();
            }

            void BinanceMdApi::connect(
                AString server,
                bool kline_stream,
                AString proxy_host,
                uint16_t proxy_port)
            {
                    this->kline_stream = kline_stream;
                    this->server = server;
                    this->proxy_host = proxy_host;
                    this->proxy_port = proxy_port;

                    // choose data host based on server selection
                    AString host;
                    if (this->server == "REAL")
                        host = REAL_DATA_HOST;
                    else
                        host = TESTNET_DATA_HOST;

                    // initialize websocket client and start
                    this->init(host, this->proxy_host, this->proxy_port, WEBSOCKET_TIMEOUT);
                    this->start();

                    this->exchange->write_log("MD API started");
            }

            void BinanceMdApi::subscribe(const SubscribeRequest& req)
            {
                // register subscription and prepare tick
                auto contract = this->exchange->get_contract_by_symbol(req.symbol);
                if (!contract)
                {
                    this->exchange->write_log(Printf("Failed to subscribe data, symbol not found: %s", req.symbol.c_str()));
                    return;
                }

                // already subscribed
                if (this->ticks.count(req.symbol))
                    return;

                this->subscribed[req.symbol] = req;

                TickData tick;
                tick.symbol = req.symbol;
                tick.exchange = req.exchange;
                tick.name = contract->name;
                tick.datetime = currentDateTime();
                tick.exchange_name = this->exchange_name;
                tick.__post_init__();
                this->ticks[req.symbol] = tick;

                // prepare channels
                AString name_lower = StrToLower(contract->name);
                std::vector<AString> channels;
                channels.push_back(name_lower + "@ticker");
                channels.push_back(name_lower + "@depth10");
                if (this->kline_stream)
                    channels.push_back(name_lower + "@kline_1m");

                // build params array
                Json params = Json::array();
                for (auto &c : channels)
                    params.push_back(c);

                this->reqid += 1;
                Json packet = {
                    {"method", "SUBSCRIBE"},
                    {"params", params},
                    {"id", this->reqid}
                };

                this->send_packet(packet);
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
                    if (!packet.contains("stream"))
                        return;

                    AString stream = packet.value("stream", "");
                    if (stream.empty())
                        return;

                    if (!packet.contains("data"))
                        return;

                    const Json& data = packet["data"];

                    size_t pos = stream.find('@');
                    if (pos == AString::npos)
                        return;

                    AString name = stream.substr(0, pos);
                    AString channel = stream.substr(pos + 1);

                    AString name_upper = StrToUpper(name);

                    auto opt_contract = this->exchange->get_contract_by_name(name_upper);
                    if (!opt_contract)
                        return;

                    ContractData contract = *opt_contract;

                    // ensure we have a tick registered for this symbol
                    if (!this->ticks.count(contract.symbol))
                        return;

                    TickData& tick = this->ticks[contract.symbol];

                    if (channel == "ticker")
                    {
                        tick.volume = JsonToFloat(data["v"]);
                        tick.turnover = JsonToFloat(data["q"]);
                        tick.open_price = JsonToFloat(data["o"]);
                        tick.high_price = JsonToFloat(data["h"]);
                        tick.low_price = JsonToFloat(data["l"]);
                        tick.last_price = JsonToFloat(data["c"]);

                        // event time
                        long long ts = 0;
                        if (data.contains("E"))
                        {
                            if (data["E"].is_number())
                                ts = data["E"].get<long long>();
                            else if (data["E"].is_string())
                                ts = std::atoll(AString(data["E"]).c_str());
                        }

                        if (ts > 0)
                            tick.datetime = DateTimeFromTimestamp(ts);

                        this->exchange->on_tick(copy(tick));
                    }
                    else if (channel == "depth10")
                    {
                        const Json& bids = data.value("b", Json::array());
                        const Json& asks = data.value("a", Json::array());

                        if (bids.is_array())
                        {
                            if (bids.size() > 0) { tick.bid_price_1 = JsonToFloat(bids[0][0]); tick.bid_volume_1 = JsonToFloat(bids[0][1]); }
                            if (bids.size() > 1) { tick.bid_price_2 = JsonToFloat(bids[1][0]); tick.bid_volume_2 = JsonToFloat(bids[1][1]); }
                            if (bids.size() > 2) { tick.bid_price_3 = JsonToFloat(bids[2][0]); tick.bid_volume_3 = JsonToFloat(bids[2][1]); }
                            if (bids.size() > 3) { tick.bid_price_4 = JsonToFloat(bids[3][0]); tick.bid_volume_4 = JsonToFloat(bids[3][1]); }
                            if (bids.size() > 4) { tick.bid_price_5 = JsonToFloat(bids[4][0]); tick.bid_volume_5 = JsonToFloat(bids[4][1]); }
                        }

                        if (asks.is_array())
                        {
                            if (asks.size() > 0) { tick.ask_price_1 = JsonToFloat(asks[0][0]); tick.ask_volume_1 = JsonToFloat(asks[0][1]); }
                            if (asks.size() > 1) { tick.ask_price_2 = JsonToFloat(asks[1][0]); tick.ask_volume_2 = JsonToFloat(asks[1][1]); }
                            if (asks.size() > 2) { tick.ask_price_3 = JsonToFloat(asks[2][0]); tick.ask_volume_3 = JsonToFloat(asks[2][1]); }
                            if (asks.size() > 3) { tick.ask_price_4 = JsonToFloat(asks[3][0]); tick.ask_volume_4 = JsonToFloat(asks[3][1]); }
                            if (asks.size() > 4) { tick.ask_price_5 = JsonToFloat(asks[4][0]); tick.ask_volume_5 = JsonToFloat(asks[4][1]); }
                        }

                        // event time
                        long long ts = 0;
                        if (data.contains("E"))
                        {
                            if (data["E"].is_number())
                                ts = data["E"].get<long long>();
                            else if (data["E"].is_string())
                                ts = std::atoll(AString(data["E"]).c_str());
                        }

                        if (ts > 0)
                            tick.datetime = DateTimeFromTimestamp(ts);

                        this->exchange->on_tick(copy(tick));
                    }
                    else
                    {
                        // ignore kline stream in C++ implementation (no extra field on TickData)
                        (void)data;
                    }

                    // final generic tick push removed to avoid duplicate pushes
                }

                void BinanceMdApi::on_error(const std::exception& ex)
                {
                    this->exchange->write_log(Printf("MD API exception: %s", ex.what()));
                }
            BinanceTradeApi::BinanceTradeApi(BinanceLinearExchange* exchange)
                : WebsocketClient()
            {
                this->exchange = exchange;
                this->exchange_name = exchange->get_exchange_name();

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

                // choose trade host based on server selection
                AString host;
                if (this->server == "REAL")
                    host = REAL_TRADE_HOST;
                else
                    host = TESTNET_TRADE_HOST;

                // initialize websocket client and start
                this->init(host, this->proxy_host, this->proxy_port, WEBSOCKET_TIMEOUT);
                this->start();

                this->exchange->write_log("Trade API started");
            }

            AString BinanceTradeApi::send_order(const OrderRequest& req)
            {
                // find contract
                auto opt_contract = this->exchange->get_contract_by_symbol(req.symbol);
                if (!opt_contract)
                {
                    this->exchange->write_log(Printf("Failed to send order, symbol not found: %s", req.symbol.c_str()));
                    return AString();
                }

                ContractData contract = *opt_contract;

                // ensure order_prefix
                if (this->order_prefix.empty())
                    this->order_prefix = DateTimeToString(currentDateTime(), "%y%m%d%H%M%S");

                // generate order id
                this->order_count += 1;
                AString orderid = this->order_prefix + std::to_string(this->order_count);

                // push submitting order
                OrderData order = req.create_order_data(orderid, this->exchange_name);
                this->exchange->on_order(order);

                // build params
                Params params;
                params["apiKey"] = this->key;
                params["symbol"] = contract.name;
                auto it_side = DIRECTION_VT2BINANCE.find(req.direction);
                if (it_side != DIRECTION_VT2BINANCE.end())
                    params["side"] = it_side->second;
                else
                    params["side"] = "BUY";
                // Round quantity to contract min_volume to avoid precision errors
                float qty = (float)req.volume;
                if (contract.min_volume > 0)
                    qty = round_to(qty, contract.min_volume);
                params["quantity"] = Printf("%g", qty).c_str();
                params["newClientOrderId"] = orderid;

                // map order type to Binance string
                auto it_type = ORDERTYPE_VT2BINANCE.find(req.type);
                AString b_type = (it_type != ORDERTYPE_VT2BINANCE.end()) ? it_type->second : AString("LIMIT");
                params["type"] = b_type;

                // stop orders include stopPrice
                if (req.type == OrderType::STOP)
                {
                    float sp = (float)req.price;
                    if (contract.pricetick > 0)
                        sp = round_to(sp, contract.pricetick);
                    params["stopPrice"] = Printf("%g", sp).c_str();
                }

                // LIMIT-like orders include price and timeInForce variations
                if (b_type == "LIMIT")
                {
                    if (req.type == OrderType::FAK)
                        params["timeInForce"] = "IOC";
                    else if (req.type == OrderType::FOK)
                        params["timeInForce"] = "FOK";
                    else
                        params["timeInForce"] = "GTC";

                    // Round price to contract pricetick to avoid precision errors
                    float fprice = (float)req.price;
                    if (contract.pricetick > 0)
                        fprice = round_to(fprice, contract.pricetick);
                    params["price"] = Printf("%g", fprice).c_str();
                }

                // sign params
                this->sign(params);

                // register callback and order mapping
                this->reqid += 1;
                this->reqid_callback_map[this->reqid] = std::bind(&BinanceTradeApi::on_send_order, this, _1);
                this->reqid_order_map[this->reqid] = order;

                // build packet
                Json params_json = Json::object();
                for (auto &p : params)
                    params_json[p.first] = p.second;

                Json packet = {
                    {"id", this->reqid},
                    {"method", "order.place"},
                    {"params", params_json}
                };

                this->send_packet(packet);

                return order.kt_orderid;
            }

            void BinanceTradeApi::cancel_order(const CancelRequest& req)
            {
                auto opt_contract = this->exchange->get_contract_by_symbol(req.symbol);
                if (!opt_contract)
                {
                    this->exchange->write_log(Printf("Failed to cancel order, symbol not found: %s", req.symbol.c_str()));
                    return;
                }

                ContractData contract = *opt_contract;

                Params params;
                params["apiKey"] = this->key;
                params["symbol"] = contract.name;
                params["origClientOrderId"] = req.orderid;

                this->sign(params);

                this->reqid += 1;
                this->reqid_callback_map[this->reqid] = std::bind(&BinanceTradeApi::on_cancel_order, this, _1);

                Json params_json = Json::object();
                for (auto &p : params)
                    params_json[p.first] = p.second;

                Json packet = {
                    {"id", this->reqid},
                    {"method", "order.cancel"},
                    {"params", params_json}
                };

                this->send_packet(packet);
            }

            void BinanceTradeApi::sign(Params& params)
            {
                // Add timestamp in milliseconds
                long long timestamp = (long long)(time(nullptr) * 1000);
                params["timestamp"] = std::to_string(timestamp);

                // Create ordered payload string from params
                std::vector<std::pair<std::string, std::string>> items;
                for (auto &p : params)
                    items.emplace_back(p.first, p.second);
                std::sort(items.begin(), items.end(), [](auto &a, auto &b){ return a.first < b.first; });

                std::string payload;
                for (size_t i = 0; i < items.size(); ++i)
                {
                    if (i) payload += "&";
                    payload += items[i].first + "=" + items[i].second;
                }

                // HMAC-SHA256 signature
                unsigned char* mac = nullptr;
                unsigned int mac_len = 0;
                int rc = HmacEncode("sha256", this->secret.c_str(), (unsigned int)this->secret.length(), payload.c_str(), (unsigned int)payload.length(), mac, mac_len);
                std::string signature;
                if (rc == 0 && mac && mac_len > 0)
                {
                    std::vector<unsigned char> outbuf(mac_len * 2 + 1);
                    hex_str(mac, mac_len, outbuf.data());
                    signature.assign((char*)outbuf.data());
                    free(mac);
                }

                params["signature"] = signature;
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
                if (!packet.contains("error"))
                    return;

                const Json& error = packet["error"];
                int error_code = error.value("code", 0);
                AString error_msg = error.value("msg", "");

                this->exchange->write_log(Printf("Order rejected, code: %d, message: %s", error_code, error_msg.c_str()));

                int id = packet.value("id", 0);
                auto it = this->reqid_order_map.find(id);
                if (it != this->reqid_order_map.end())
                {
                    OrderData order = it->second;
                    order.status = Status::REJECTED;
                    this->exchange->on_order(order);
                }
            }

            void BinanceTradeApi::on_cancel_order(const Json& packet)
            {
                if (!packet.contains("error"))
                    return;

                const Json& error = packet["error"];
                int error_code = error.value("code", 0);
                AString error_msg = error.value("msg", "");

                this->exchange->write_log(Printf("Cancel rejected, code: %d, message: %s", error_code, error_msg.c_str()));
            }

            void BinanceTradeApi::on_error(const std::exception& ex)
            {
                this->exchange->write_log(Printf("Trade API exception: %s", ex.what()));
            }

            BinanceUserApi::BinanceUserApi(BinanceLinearExchange* exchange)
                : WebsocketClient()
            {
                this->exchange = exchange;
                this->exchange_name = exchange->get_exchange_name();
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
                if (!packet.contains("a"))
                    return;

                const Json& a = packet["a"];

                // balances
                if (a.contains("B") && a["B"].is_array())
                {
                    for (const Json& acc : a["B"]) {
                        AccountData account;
                        account.accountid = acc.value("a", "");
                        account.balance = JsonToFloat(acc.value("wb", 0.0));
                        double cw = JsonToFloat(acc.value("cw", 0.0));
                        account.frozen = account.balance - cw;
                        account.exchange_name = this->exchange_name;
                        account.__post_init__();

                        if (account.balance != 0.0)
                            this->exchange->on_account(account);
                    }
                }

                // positions
                if (a.contains("P") && a["P"].is_array())
                {
                    for (const Json& pos : a["P"]) {
                        AString ps = pos.value("ps", "");
                        if (ps != "BOTH")
                            continue;

                        // volume may be string or number
                        double volume = 0.0;
                        if (pos.contains("pa"))
                            volume = JsonToFloat(pos["pa"]);

                        AString name = pos.value("s", "");
                        auto opt_contract = this->exchange->get_contract_by_name(name);
                        if (!opt_contract)
                            continue;

                        ContractData contract = *opt_contract;

                        PositionData position;
                        position.symbol = contract.symbol;
                        position.exchange = Exchange::BINANCE;
                        position.direction = Direction::NET;
                        position.volume = (float)volume;
                        position.price = JsonToFloat(pos.value("ep", 0.0));
                        position.pnl = JsonToFloat(pos.value("up", 0.0));
                        position.exchange_name = this->exchange_name;
                        position.__post_init__();

                        this->exchange->on_position(position);
                    }
                }
            }

            void BinanceUserApi::on_order(const Json& packet)
            {
                if (!packet.contains("o"))
                    return;

                const Json& ord = packet["o"];

                // determine order type using mapping
                AString type = ord.value("o", "");
                AString tif = ord.value("f", "GTC");
                OrderType order_type = OrderType::LIMIT;
                auto it_ot = ORDERTYPE_BINANCE2VT.find(type);
                if (it_ot != ORDERTYPE_BINANCE2VT.end())
                    order_type = it_ot->second;
                // adjust LIMIT by timeInForce
                if (order_type == OrderType::LIMIT)
                {
                    if (tif == "IOC") order_type = OrderType::FAK;
                    else if (tif == "FOK") order_type = OrderType::FOK;
                }

                // find contract by exchange name
                AString name = ord.value("s", "");
                auto opt_contract = this->exchange->get_contract_by_name(name);
                if (!opt_contract)
                    return;

                ContractData contract = *opt_contract;

                // build order
                OrderData order;
                order.orderid = ord.value("c", "");
                order.symbol = contract.symbol;
                order.exchange = Exchange::BINANCE;
                order.price = JsonToFloat(ord.value("p", 0.0));
                order.volume = JsonToFloat(ord.value("q", 0.0));
                order.traded = JsonToFloat(ord.value("z", 0.0));
                order.type = order_type;
                AString side = ord.value("S", "");
                auto it_dir = DIRECTION_BINANCE2VT.find(side);
                if (it_dir != DIRECTION_BINANCE2VT.end())
                    order.direction = it_dir->second;
                else
                    order.direction = Direction::LONG;

                // map status using lookup table
                AString status = ord.value("X", "");
                auto it_status = STATUS_BINANCE2KT.find(status);
                if (it_status != STATUS_BINANCE2KT.end())
                    order.status = it_status->second;
                else
                    order.status = Status::NOTTRADED;

                long long evt = packet.value("E", 0LL);
                if (evt > 0)
                    order.datetime = DateTimeFromTimestamp(evt);
                else
                    order.datetime = currentDateTime();

                order.exchange_name = this->exchange_name;
                order.__post_init__();

                this->exchange->on_order(order);

                // trades
                double trade_volume = JsonToFloat(ord.value("l", 0.0));
                trade_volume = round_to((float)trade_volume, contract.min_volume);
                if (trade_volume <= 0.0)
                    return;

                TradeData trade;
                trade.symbol = order.symbol;
                trade.exchange = order.exchange;
                trade.orderid = order.orderid;
                trade.tradeid = ord.value("t", "");
                trade.direction = order.direction;
                trade.price = JsonToFloat(ord.value("L", 0.0));
                trade.volume = (float)trade_volume;
                long long ttime = ord.value("T", 0LL);
                if (ttime > 0)
                    trade.datetime = DateTimeFromTimestamp(ttime);
                else
                    trade.datetime = currentDateTime();
                trade.exchange_name = this->exchange_name;
                trade.__post_init__();

                this->exchange->on_trade(trade);
            }

            void BinanceUserApi::on_disconnected(int status_code, const AString& msg)
            {
                this->exchange->write_log(Printf("User API disconnected, code: %d, msg: %s", status_code, msg.c_str()));
                this->exchange->rest_api->start_user_stream();
            }
        }
    }
}