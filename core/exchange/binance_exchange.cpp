#include <api/Globals.h>
#include <engine/engine.h>
#include <engine/utility.h>
#include <event/event.h>
#include "binance_exchange.h"
#include "algo_hmac.h"
#include "utils.h"

using namespace std::placeholders;


namespace Keen
{
	namespace exchange
	{
		namespace binance
		{
			const AString REST_HOST = "https://www.binance.com";
			const AString WEBSOCKET_TRADE_HOST = "wss://stream.binance.com:9443/ws/";
			const AString WEBSOCKET_DATA_HOST = "wss://stream.binance.com:9443/stream";

			const AString TESTNET_REST_HOST = "https://testnet.binance.vision";
			const AString TESTNET_WEBSOCKET_TRADE_HOST = "wss://testnet.binance.vision/ws/";
			const AString TESTNET_WEBSOCKET_DATA_HOST = "wss://testnet.binance.vision/stream";

			AString generate_signature(AString msg, AString secret_key);

			std::map<AString, Status> STATUS_BINANCE2KT = {
				{"NEW", Status::NOTTRADED},
				{"PARTIALLY_FILLED", Status::PARTTRADED},
				{"FILLED", Status::ALLTRADED},
				{"CANCELED", Status::CANCELLED},
				{"REJECTED", Status::REJECTED},
				{"EXPIRED", Status::CANCELLED},
			};

			std::map<OrderType, AString> ORDERTYPE_KT2BINANCE = {
				{OrderType::LIMIT, "LIMIT"},
				{OrderType::MARKET, "MARKET"},
				{OrderType::STOP, "STOP_LOSS"}
			};

			std::map<AString, OrderType> ORDERTYPE_BINANCE2KT;

			std::map<Direction, AString> DIRECTION_KT2BINANCE = {
				{Direction::LONG, "BUY"},
				{Direction::SHORT, "SELL"}};
			std::map<AString, Direction> DIRECTION_BINANCE2KT;

			std::map<Interval, AString> INTERVAL_KT2BINANCE = {
				{Interval::MINUTE, "1m"},
				{Interval::HOUR, "1h"},
				{Interval::DAILY, "1d"},
			};

			std::map<Interval, std::chrono::minutes> TIMEDELTA_MAP = {
				{Interval::MINUTE, std::chrono::minutes{1}},
				{Interval::HOUR, std::chrono::hours{1}},
				{Interval::DAILY, std::chrono::days{1}}
			};

			std::map<AString, ContractData> symbol_name_map = {};

			enum Security
			{
				NONE = 0,
				SIGNED = 1,
				API_KEY = 2,
			};

			void InitGlobals()
			{
				for (auto &[k, v] : ORDERTYPE_KT2BINANCE)
				{
					ORDERTYPE_BINANCE2KT[v] = k;
				}

				for (auto &[k, v] : DIRECTION_KT2BINANCE)
				{
					DIRECTION_BINANCE2KT[v] = k;
				}
			}

			BinanceExchange::BinanceExchange(EventEmitter* event_emitter)
				: BaseExchange(event_emitter, "BINANCE")
			{
				InitGlobals();

				default_setting = {
					{"api_key", ""},
					{"api_secret", ""},
					{"server", "REAL"}, //["REAL", "TESTNET"]
					{"kline_stream", true}, //["False", "True"],
					{"proxy_host", "" },
					{"proxy_port", 0},
				};

				exchange = Exchange::BINANCE;

				this->trade_ws_api = new BinanceTradeWebsocketApi(this);
				this->market_ws_api = new BinanceDataWebsocketApi(this);
				this->rest_api = new BinanceRestApi(this);
			}

			BinanceExchange::~BinanceExchange()
			{
				SAFE_RELEASE(trade_ws_api);
				SAFE_RELEASE(market_ws_api);
				SAFE_RELEASE(rest_api);
			}

			void BinanceExchange::connect(const Json &setting)
			{
				AString key = setting.value("api_key","");
				AString secret = setting.value("api_secret", "");
				AString server = setting.value("server", "");
				bool kline_stream = setting.value("kline_stream", true);
				AString proxy_host = setting.value("proxy_host", "");
				int proxy_port = setting.value("proxy_port", 0);

				this->rest_api->connect(key, secret, server, proxy_host, proxy_port);
				this->market_ws_api->connect(proxy_host, proxy_port);
				this->event_emitter->Register(engine::EVENT_TIMER, std::bind(&BinanceExchange::process_timer_event, this, _1));
			}

			void BinanceExchange::subscribe(const SubscribeRequest &req)
			{
				this->market_ws_api->subscribe(req);
			}

			AString BinanceExchange::send_order(const OrderRequest &req)
			{
				return this->rest_api->send_order(req);
			}

			void BinanceExchange::cancel_order(const CancelRequest& req)
			{
				this->rest_api->cancel_order(req);
			}

			void BinanceExchange::query_account()
			{
			}

			void BinanceExchange::query_position()
			{
			}

			std::list<BarData> BinanceExchange::query_history(const HistoryRequest& req)
			{
				return this->rest_api->query_history(req);
			}

			void BinanceExchange::close()
			{
				this->rest_api->stop();
				this->trade_ws_api->stop();
				this->market_ws_api->stop();
			}

			void BinanceExchange::process_timer_event(const Event &event)
			{
				this->rest_api->keep_user_stream();
			}

			BinanceRestApi::BinanceRestApi(BinanceExchange *exchange)
				: RestClient()
			{
				this->exchange = exchange;
				this->exchange_name = exchange->exchange_name;

				this->trade_ws_api = this->exchange->trade_ws_api;

				this->key = "";
				this->secret = "";

				this->user_stream_key = "";
				this->keep_alive_count = 0;
				this->recv_window = 5000;
				this->time_offset = 0;

				this->order_count = 1000000;
				this->connect_time = 0;
			}

			Request &BinanceRestApi::sign(Request &request)
			{
				/*
				 * Generate BINANCE signature.
				 */
				Security security = std::get<Json>(request.data)["security"];
				if (security == Security::NONE)
				{
					request.data = AString();
					return request;
				}

				AString path;
				if (request.params.size())
				{
					path = request.path + "?" + BuildParams(request.params);
				}
				else
				{
					request.params = {};
					path = request.path;
				}

				if (security == Security::SIGNED)
				{
					int64_t timestamp = currentDateTime().time_since_epoch().count();

					if (this->time_offset > 0)
						timestamp -= abs(this->time_offset);
					else if (this->time_offset < 0)
						timestamp += abs(this->time_offset);

					request.params["timestamp"] = std::to_string(timestamp);

					AString query = BuildParams(request.params);
					AString signature = generate_signature(query, this->secret);

					query += "&signature=" + signature;
					path = request.path + "?" + query;
				}

				request.path = path;
				request.params = {};
				request.data = AString();

				// Add headers
				Json headers = {
					{"Content-Type", "application/x-www-form-urlencoded"},
					{"Accept", "application/json"},
					{"X-MBX-APIKEY", this->key}};

				static std::set<Security> Security = {Security::SIGNED, Security::API_KEY};

				if (Security.count(security))
					request.headers = headers;

				return request;
			}

			void BinanceRestApi::connect(
				AString key,
				AString secret,
				AString server,
				AString proxy_host,
				uint16_t proxy_port)
			{
				/*
				 * Initialize connection to REST server.
				 */
				this->key = key;
				this->secret = secret;
				this->proxy_port = proxy_port;
				this->proxy_host = proxy_host;
				this->server = server;

				this->connect_time = time_point_cast<seconds>(currentDateTime()).time_since_epoch().count() * this->order_count;

				if(this->server == "REAL")
					this->init(REST_HOST, proxy_host, proxy_port);
				else
					this->init(TESTNET_REST_HOST, proxy_host, proxy_port);
				this->start();

				this->exchange->write_log("REST API started");

				this->query_time();
				this->query_account();
				this->query_order();
				this->query_contract();
				this->start_user_stream();
			}

			void BinanceRestApi::query_time()
			{
				Json data = {{"security", Security::NONE}};
				AString path = "/api/v1/time";

				Request request{ 
					.method = "GET",
					.path = "/api/v1/time",
					.data = data,
					.callback = std::bind(&BinanceRestApi::on_query_time, this, _1, _2)
				};

				this->request(request);
			}

			void BinanceRestApi::query_account()
			{
				Json data = {{"security", Security::SIGNED}};

				Request request{
					.method = "GET",
					.path = "/api/v3/account",
					.data = data,
					.callback = std::bind(&BinanceRestApi::on_query_account, this, _1, _2)
				};

				this->request(request);
			}

			void BinanceRestApi::query_order()
			{
				Json data = { {"security", Security::SIGNED} };

				Request request{
					.method = "GET",
					.path = "/api/v3/openOrders",
					.data = data,
					.callback = std::bind(&BinanceRestApi::on_query_order, this, _1, _2)
				};

				this->request(request);
			}

			void BinanceRestApi::query_contract()
			{
				Json data = {{"security", Security::NONE}};
	
				Request request{
					.method = "GET",
					.path = "/api/v1/exchangeInfo",
					.data = data,
					.callback = std::bind(&BinanceRestApi::on_query_contract, this, _1, _2)
				};

				this->request(request);
			}

			int BinanceRestApi::_new_order_id()
			{
				cCSLock Lock(order_count_lock);
				this->order_count += 1;
				return this->order_count;
			}

			AString BinanceRestApi::send_order(const OrderRequest &req)
			{
				AString orderid = Printf("a%lld%d", this->connect_time, this->_new_order_id());
				OrderData order = req.create_order_data(
					orderid, this->exchange_name);

				this->exchange->on_order(order);

				Json data = {{"security", Security::SIGNED}};

				Params params = {
					{"symbol", req.symbol},
					{"timeInForce", "GTC"},
					{"side", DIRECTION_KT2BINANCE[req.direction]},
					{"type", ORDERTYPE_KT2BINANCE[req.type]},
					{"price", std::to_string(req.price)},
					{"quantity", std::to_string(req.volume)},
					{"newClientOrderId", orderid},
					{"newOrderRespType", "ACK"}
				};

				Request request{
					.method = "POST",
					.path = "/api/v3/order",
					.params = params,
					.data = data,
					.extra = order,
					.callback = std::bind(&BinanceRestApi::on_send_order, this, _1, _2),
					.on_failed = std::bind(&BinanceRestApi::on_send_order_failed, this, _1, _2),
					.on_error = std::bind(&BinanceRestApi::on_send_order_error, this, _1, _2)
				};

				this->request(request);

				return order.kt_orderid;
			}

			void BinanceRestApi::cancel_order(CancelRequest req)
			{
				Json data = {{"security", Security::SIGNED}};

				Params params = {
					{"symbol", req.symbol},
					{"origClientOrderId", req.orderid}
				};

				Request request{
					.method = "DELETE",
					.path = "/api/v3/order",
					.params = params,
					.data = data,
					.extra = req,
					.callback = std::bind(&BinanceRestApi::on_cancel_order, this, _1, _2)
				};

				this->request(request);
			}

			void BinanceRestApi::start_user_stream()
			{
				Json data = {{"security", Security::API_KEY}};

				Request request{
					.method = "POST",
					.path = "/api/v1/userDataStream",
					.data = data,
					.callback = std::bind(&BinanceRestApi::on_start_user_stream, this, _1, _2)
				};

				this->request(request);
			}

			void BinanceRestApi::keep_user_stream()
			{
				this->keep_alive_count += 1;
				if (this->keep_alive_count < 1800)
					return;

				Json data = {{"security", Security::API_KEY}};

				Params params = {
					{"listenKey", this->user_stream_key}
				};

				Request request{
					.method = "PUT",
					.path = "/api/v1/userDataStream",
					.params = params,
					.data = data,
					.callback = std::bind(&BinanceRestApi::on_keep_user_stream, this, _1, _2)
				};

				this->request(request);
			}

			void BinanceRestApi::on_query_time(const Json &data, const Request& request)
			{
				int64_t local_time = currentDateTime().time_since_epoch().count();
				int64_t server_time = data["serverTime"].get<int64_t>();
				this->time_offset = int(local_time - server_time);
			}

			void BinanceRestApi::on_query_account(const Json &data, const Request& request)
			{
				for (Json account_data : data["balances"])
				{
					AccountData account = AccountData(
						/*account_data["asset"],
						JsonToFloat(account_data["free"]) + JsonToFloat(account_data["locked"]),
						JsonToFloat(account_data["locked"]),
						this->exchange_name*/
					);

					if (account.balance)
						this->exchange->on_account(account);
				}

				this->exchange->write_log("Account funds query successful");
			}

			void BinanceRestApi::on_query_order(const Json &data, const Request& request)
			{
				for (Json d : data)
				{
					DateTime time = DateTimeFromTimestamp(d["time"]);

					OrderData order /*(
						 d["symbol"],
						 Exchange::BINANCE,
						 ORDERTYPE_BINANCE2KT[d["type"]],
						 d["clientOrderId"],
						 DIRECTION_BINANCE2KT[d["side"]],
						 Offset::NONE,
						 JsonToFloat(d["price"]),
						 JsonToFloat(d["origQty"]),
						 JsonToFloat(d["executedQty"]),
						 time,
						 STATUS_BINANCE2KT[d["status"]],
						 this->exchange_name
						 )*/
						;
					this->exchange->on_order(order);
				}

				this->exchange->write_log("Entrustment information query successful");
			}

			void BinanceRestApi::on_query_contract(const Json &data, const Request& request)
			{
				for (Json d : data["symbols"])
				{
					AString base_currency = StrToUpper(d["baseAsset"]);
					AString quote_currency = StrToUpper(d["quoteAsset"]);
					AString name = Printf("%s/%s", base_currency.c_str(), quote_currency.c_str());

					float pricetick = 1;
					float min_volume = 1;

					for (Json f : d["filters"])
					{
						if (f["filterType"] == "PRICE_FILTER")
							pricetick = JsonToFloat(f["tickSize"]);
						else if (f["filterType"] == "LOT_SIZE")
							min_volume = JsonToFloat(f["stepSize"]);
					}

					ContractData contract = ContractData(
						/*d["symbol"],
						Exchange::BINANCE,
						name,
						Product::SPOT,
						1,
						pricetick,
						min_volume,
						true,
						this->exchange_name*/
					);
					this->exchange->on_contract(contract);

					symbol_name_map[contract.symbol] = contract;
				}

				this->exchange->write_log("Contract information query successful");
			}

			void BinanceRestApi::on_send_order(const Json &data, const Request& request)
			{
			}

			void BinanceRestApi::on_send_order_failed(int status_code, const Request& request)
			{
				OrderData order = std::any_cast<OrderData>(request.extra);
				order.status = Status::REJECTED;
				this->exchange->on_order(order);

				AString msg = Printf("Commission failed, status code: %d, message: %s",
					status_code, request.response->body.c_str());
				this->exchange->write_log(msg);
			}

			void BinanceRestApi::on_send_order_error(const std::exception &ex, const Request& request)
			{
				OrderData order = std::any_cast<OrderData>(request.extra);
				order.status = Status::REJECTED;
				this->exchange->on_order(order);

				// Record exception if not ConnectionError
				// 				if not issubclass(exception_type, ConnectionError) {
				// 					this->on_error(exception_type, exception_value, tb, request);
				// 				}
			}

			void BinanceRestApi::on_cancel_order(const Json &data, const Request& request)
			{
			}

			void BinanceRestApi::on_start_user_stream(const Json &data, const Request& request)
			{
				this->user_stream_key = data["listenKey"];
				this->keep_alive_count = 0;
				AString url = WEBSOCKET_TRADE_HOST + this->user_stream_key;

				this->trade_ws_api->connect(url, this->proxy_host, this->proxy_port);
			}

			void BinanceRestApi::on_keep_user_stream(const Json &data, const Request& request)
			{
			}

			std::list<BarData> BinanceRestApi::query_history(HistoryRequest req)
			{
				std::list<BarData> history = {};
				int limit = 1000;
				auto start_time = req.start;

				while (true)
				{
					// Create query params
					Json params = {
						{"symbol", req.symbol},
						{"interval", INTERVAL_KT2BINANCE[req.interval]},
						{"limit", limit},
						{"startTime", start_time.time_since_epoch().count()} // convert to millisecond
					};

					// Add end time if specified
					if (req.end.time_since_epoch().count())
					{
						// convert to millisecond
						auto end_time = req.end.time_since_epoch().count();
						params["endTime"] = end_time;
					}

					// Get response from server
					Response resp = this->request(
						"GET",
						"/api/v1/klines",
						params,
						{{"security", Security::NONE}});

					// Break if request failed with other status code
					if (resp.status / 100 != 2)
					{
						AString msg = Printf("Failed to obtain historical data, status code: %d, information: %s", resp.status,
											 resp.body.c_str());
						this->exchange->write_log(msg);
						break;
					}
					else
					{
						Json data = Json::parse(resp.body); // get() is needed!
						if (data.is_null())
						{
							AString msg = Printf("The historical data is empty, start time: %s", DateTimeToString(start_time).c_str());
							this->exchange->write_log(msg);
							break;
						}

						std::list<BarData> buf;

						for (auto l : data)
						{

							BarData bar;
							bar.symbol = req.symbol;
							bar.exchange = req.exchange;
							bar.datetime = DateTimeFromTimestamp(l[0]);
							bar.interval = req.interval;
							bar.volume = JsonToFloat(l[5]);
							bar.open_price = JsonToFloat(l[1]);
							bar.high_price = JsonToFloat(l[2]);
							bar.low_price = JsonToFloat(l[3]);
							bar.close_price = JsonToFloat(l[4]);
							bar.exchange_name = this->exchange_name;

							bar.__post_init__();

							buf.push_back(bar);
						}

						history.insert(history.end(), buf.begin(), buf.end());

						DateTime begin = buf.front().datetime;
						DateTime end = buf.back().datetime;
						AString msg = Printf("Successfully obtained historical data. %s - %s，%s - %s",
											 req.symbol.c_str(), interval_to_str(req.interval).c_str(),
											 DateTimeToString(begin).c_str(), DateTimeToString(end).c_str());
						this->exchange->write_log(msg);

						// Break if total data count less than limit(latest date collected)
						if (data.size() < limit)
							break;

						// Update start time
						auto start_dt = buf.back().datetime + TIMEDELTA_MAP[req.interval];
						start_time = start_dt;
					}
				}

				return history;
			}

			BinanceTradeWebsocketApi::BinanceTradeWebsocketApi(BinanceExchange *exchange)
				: WebsocketClient()
			{
				this->exchange = exchange;
				this->exchange_name = exchange->exchange_name;
			}

			void BinanceTradeWebsocketApi::connect(AString url, AString proxy_host, uint16_t proxy_port)
			{
				this->init(url, proxy_host, proxy_port);
				this->start();
			}

			void BinanceTradeWebsocketApi::on_connected()
			{
				this->exchange->write_log("Transaction Websocket API connection successful");
			}

			void BinanceTradeWebsocketApi::on_packet(const Json& packet)
			{
				if (packet["e"] == "outboundAccountInfo")
					this->on_account(packet);
				else if (packet["e"] == "executionReport")
					this->on_order(packet);
			}

			void BinanceTradeWebsocketApi::on_account(const Json &packet)
			{
				for (Json d : packet["B"])
				{
					AccountData account = AccountData(
						/*d["a"],
						JsonToFloat(d["f"]) + JsonToFloat(d["l"]),
						JsonToFloat(d["l"]),
						this->exchange_name*/
					);

					if (account.balance)
						this->exchange->on_account(account);
				}
			}

			void BinanceTradeWebsocketApi::on_order(const Json &packet)
			{
				DateTime time = DateTimeFromTimestamp(packet["O"]);

				AString orderid;
				if (packet["C"] == "null")
					orderid = packet["c"];
				else
					orderid = packet["C"];

				OrderData order /*(
					 packet["s"],
					 Exchange::BINANCE,
					 ORDERTYPE_BINANCE2KT[packet["o"]],
					 orderid,
					 DIRECTION_BINANCE2KT[packet["S"]],
					 Offset::NONE,
					 JsonToFloat(packet["p"]),
					 JsonToFloat(packet["q"]),
					 JsonToFloat(packet["z"]),
					 time,
					 STATUS_BINANCE2KT[packet["X"]],
					 this->exchange_name
				 )*/;

				this->exchange->on_order(order);

				// Push trade event
				float trade_volume = JsonToFloat(packet["l"]);
				if (!trade_volume)
					return;

				DateTime trade_time = DateTimeFromTimestamp(packet["T"]);

				TradeData trade{
					.symbol = order.symbol,
					.exchange = order.exchange,
					.orderid = order.orderid,
					.tradeid = packet["t"],
					.direction = order.direction,
					.offset = Offset::NONE,
					.price = JsonToFloat(packet["L"]),
					.volume = trade_volume,
					.datetime = trade_time,
					.exchange_name = this->exchange_name};
				trade.__post_init__();

				this->exchange->on_trade(trade);
			}

			BinanceDataWebsocketApi::BinanceDataWebsocketApi(BinanceExchange *exchange)
				: WebsocketClient()
			{
				this->exchange = exchange;
				this->exchange_name = exchange->exchange_name;

				this->ticks = {};
			}

			void BinanceDataWebsocketApi::connect(AString proxy_host, uint16_t proxy_port)
			{
				this->proxy_host = proxy_host;
				this->proxy_port = proxy_port;
			}

			void BinanceDataWebsocketApi::on_connected()
			{
				this->exchange->write_log("Market Websocket API connection refresh");
			}

			void BinanceDataWebsocketApi::subscribe(const SubscribeRequest& req)
			{
				if (!symbol_name_map.count(req.symbol))
				{
					this->exchange->write_log(Printf("The contract code could not be found %s", req.symbol.c_str()));
					return;
				}

				// Create tick buf data
				TickData tick = TickData(
					/*req.symbol,
					Exchange::BINANCE,
					GetWithDefault(symbol_name_map, req.symbol, AString("")),
					currentDateTime(),
					this->exchange_name*/
				);

				this->ticks[StrToLower(req.symbol)] = tick;

				// Close previous connection
				if (this->is_active())
				{
					this->stop();
				}

				// Create new connection
				AStringVector channels;
				for (AString ws_symbol : MapKeys(this->ticks))
				{
					channels.push_back(ws_symbol + "@ticker");
					channels.push_back(ws_symbol + "@depth5");
				}

				AString url = WEBSOCKET_DATA_HOST + StringsConcat(channels, '/');
				this->init(url, this->proxy_host, this->proxy_port);
				this->start();
			}

			void BinanceDataWebsocketApi::on_packet(Json packet)
			{
				AString stream = packet["stream"];
				Json data = packet["data"];

				AStringVector ret = StringSplit(stream, "@");
				AString symbol = ret[0];
				AString channel = ret[1];

				TickData &tick = this->ticks[symbol];

				if (channel == "ticker")
				{
					tick.volume = JsonToFloat(data["v"]);
					tick.open_price = JsonToFloat(data["o"]);
					tick.high_price = JsonToFloat(data["h"]);
					tick.low_price = JsonToFloat(data["l"]);
					tick.last_price = JsonToFloat(data["c"]);
					tick.datetime = DateTimeFromTimestamp(data["E"]);
				}
				else
				{
					Json bids = data["bids"];
					tick.bid_price_1 = JsonToFloat(bids[0][0]);
					tick.bid_volume_1 = JsonToFloat(bids[0][1]);
					tick.bid_price_2 = JsonToFloat(bids[1][0]);
					tick.bid_volume_2 = JsonToFloat(bids[1][1]);
					tick.bid_price_3 = JsonToFloat(bids[2][0]);
					tick.bid_volume_3 = JsonToFloat(bids[2][1]);
					tick.bid_price_4 = JsonToFloat(bids[3][0]);
					tick.bid_volume_4 = JsonToFloat(bids[3][1]);
					tick.bid_price_5 = JsonToFloat(bids[4][0]);
					tick.bid_volume_5 = JsonToFloat(bids[4][1]);

					Json asks = data["asks"];
					tick.ask_price_1 = JsonToFloat(asks[0][0]);
					tick.ask_volume_1 = JsonToFloat(asks[0][1]);
					tick.ask_price_2 = JsonToFloat(asks[1][0]);
					tick.ask_volume_2 = JsonToFloat(asks[1][1]);
					tick.ask_price_3 = JsonToFloat(asks[2][0]);
					tick.ask_volume_3 = JsonToFloat(asks[2][1]);
					tick.ask_price_4 = JsonToFloat(asks[3][0]);
					tick.ask_volume_4 = JsonToFloat(asks[3][1]);
					tick.ask_price_5 = JsonToFloat(asks[4][0]);
					tick.ask_volume_5 = JsonToFloat(asks[4][1]);
				}

				if (tick.last_price)
					this->exchange->on_tick(copy(tick));
			}

			AString HexDigest(const char *a_RawData, size_t a_NumShorts)
			{
				static const char HEX[] = "0123456789abcdef";

				AString ret;

				for (size_t i = 0; i != a_NumShorts; ++i)
				{
					ret.push_back(HEX[(a_RawData[i] >> 4) & 0x0f]);
					ret.push_back(HEX[a_RawData[i] & 0x0f]);
				}
				return ret;
			}

			AString generate_signature(AString msg, AString secret_key)
			{
				AString sign;
				unsigned char *mac = NULL;
				unsigned int mac_length = 0;
				AString key = secret_key;
				int ret = HmacEncode("sha256", key.c_str(), key.length(), msg.c_str(), msg.length(), mac, mac_length);
				sign = HexDigest((char *)mac, mac_length);
				return sign;
			}
		}
	}
}
