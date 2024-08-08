#include <api/Globals.h>
#include <engine/engine.h>
#include <engine/utility.h>
#include "okx_exchange.h"
#include "algo_hmac.h"
#include "utils.h"


using namespace std::placeholders;

#define  BUFLEN 65536

namespace Keen
{
	namespace exchange
	{
		namespace okx
		{
			const AString REST_HOST = "https://www.okx.com";

			const AString PUBLIC_WEBSOCKET_HOST = "wss://ws.okx.com:8443/ws/v5/public";
			const AString PRIVATE_WEBSOCKET_HOST = "wss://ws.okx.com:8443/ws/v5/private";

			const AString TEST_PUBLIC_WEBSOCKET_HOST = "wss://wspap.okx.com:8443/ws/v5/public?brokerId=9999";
			const AString TEST_PRIVATE_WEBSOCKET_HOST = "wss://wspap.okx.com:8443/ws/v5/private?brokerId=9999";


			AString okx_generate_signature(AString msg, AString secret_key);

			AString generate_timestamp();


			static std::map<AString, Status> STATUS_OKX2KT = {
				{ "live", Status::NOTTRADED },
				{ "partially_filled", Status::PARTTRADED },
				{ "filled", Status::ALLTRADED },
				{ "canceled", Status::CANCELLED }
			};

			static std::map<AString, OrderType> ORDERTYPE_OKX2KT = {
				{ "limit", OrderType::LIMIT },
				{ "market", OrderType::MARKET },
				{ "fok", OrderType::FOK },
				{ "ioc", OrderType::FAK }
			};

			static std::map<OrderType, AString> ORDERTYPE_KT2OKX = {
				{ OrderType::LIMIT, "limit" },
				{ OrderType::MARKET, "market" },
				{ OrderType::FOK, "fok" },
				{ OrderType::FAK, "ioc" }
			};

			static std::map<AString, Direction> DIRECTION_OKX2KT = {
				{ "buy", Direction::LONG },
				{ "sell", Direction::SHORT }
			};

			static std::map<Direction, AString> DIRECTION_KT2OKX = {
					{ Direction::LONG, "buy" },
					{ Direction::SHORT, "sell" }
			};

			static std::map<Direction, AString> POSSIZE_KT2OKX = {
					{ Direction::LONG, "long" },
					{ Direction::SHORT, "short" },
					{ Direction::NET, "net" }
			};

			static std::map<Interval, AString> INTERVAL_KT2OKX = {
				{ Interval::MINUTE, "1m" },
				{ Interval::HOUR, "1H" },
				{ Interval::DAILY, "1D" }
			};

			static std::map<AString, Product> PRODUCT_OKX2KT = {
				{ "SWAP", Product::FUTURES },
				{ "SPOT", Product::SPOT },
				{ "FUTURES", Product::FUTURES },
				{ "OPTION", Product::OPTION }
			};

			static std::map<Product, AString> PRODUCT_KT2OKX = {
				{ Product::FUTURES, "SWAP" },
				{ Product::SPOT, "SPOT" },
				{ Product::FUTURES, "FUTURES" },
				{ Product::OPTION, "OPTION" }
			};


			static std::map<AString, OptionType> OPTIONTYPE_OKXO2KT = {
				{ "C", OptionType::CALL },
				{ "P" , OptionType::PUT }
			};

			static std::map<AString, ContractData> symbol_contract_map;
			static AStringSet local_orderids;

			OrderData parse_order_data(const Json& data, AString exchange_name);

			float get_float_value(const Json& data, AString key);


			OkxExchange::OkxExchange(EventEmitter* event_emitter)
				: BaseExchange(event_emitter, "OKX")
			{
				default_setting =
				{
					{ "API Key", "" },
					{ "Secret Key", "" },
					{ "Passphrase", "" },
					{ "Proxy Host", "" },
					{ "Proxy Port", 0 },
					{ "Server", "" } //["REAL", "TEST"]
				};

				exchange = Exchange::OKX;

				this->rest_api = new OkxRestApi(this);
				this->ws_public_api = new OkxWebsocketPublicApi(this);
				this->ws_private_api = new OkxWebsocketPrivateApi(this);
			}

			OkxExchange::~OkxExchange()
			{
				SAFE_RELEASE(rest_api);
				SAFE_RELEASE(ws_public_api);
				SAFE_RELEASE(ws_private_api);
			}

			void OkxExchange::connect(const Json& setting)
			{
				AString key = setting.value("API Key", "");
				AString secret = setting.value("Secret Key", "");
				AString passphrase = setting.value("Passphrase", "");

				AString proxy_host = setting.value("Proxy Host", "");
				uint16_t proxy_port = setting.value("Proxy Port", 0);
				AString server = setting.value("Server", "");

				this->rest_api->connect(key,
					secret,
					passphrase,
					proxy_host,
					proxy_port,
					server);

				this->ws_public_api->connect(
					proxy_host,
					proxy_port,
					server
				);
				this->ws_private_api->connect(
					key,
					secret,
					passphrase,
					proxy_host,
					proxy_port,
					server
				);
			}

			void OkxExchange::subscribe(const SubscribeRequest& req)
			{
				this->ws_public_api->subscribe(req);
			}

			AString OkxExchange::send_order(const OrderRequest& req)
			{
				return this->ws_private_api->send_order(req);
			}

			void OkxExchange::cancel_order(const CancelRequest& req)
			{
				this->ws_private_api->cancel_order(req);
			}

			void OkxExchange::query_account()
			{
			}

			void OkxExchange::query_position()
			{
			}

			std::list<BarData> OkxExchange::query_history(const HistoryRequest& req)
			{
				return this->rest_api->query_history(req);
			}

			void OkxExchange::close()
			{
				this->rest_api->stop();
				this->ws_public_api->stop();
				this->ws_private_api->stop();
			}

			void OkxExchange::on_order(const OrderData& order)
			{
				this->orders[order.orderid] = order;
				BaseExchange::on_order(order);
			}

			std::optional<OrderData> OkxExchange::get_order(AString orderid)
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





			OkxRestApi::OkxRestApi(OkxExchange* exchange)
				: RestClient()
			{

				this->exchange = exchange;
				this->exchange_name = exchange->exchange_name;

				this->key = "";
				this->secret = "";
				this->passphrase = "";
				this->simulated = false;
			}

			Request& OkxRestApi::sign(Request& request)
			{
				AString timestamp = generate_timestamp();

				AString request_data = RequestDataToString(request.data);

				AString path;
				if (request.params.size())
					path = request.path + '?' + /*URLEncode */BuildParams(request.params);
				else
					path = request.path;

				AString msg = timestamp + request.method + path + request_data;
				AString signature = okx_generate_signature(msg, this->secret);

				// Add headers
				request.headers = {
					{ "OK-ACCESS-KEY", this->key },
					{ "OK-ACCESS-SIGN" , signature },
					{ "OK-ACCESS-TIMESTAMP" , timestamp },
					{ "OK-ACCESS-PASSPHRASE" , this->passphrase },
					{ "Content-Type" , "application/json" }
				};

				if (this->simulated)
					request.headers.insert(std::pair("x-simulated-trading", "1"));

				return request;
			}

			void OkxRestApi::connect(
				AString key,
				AString secret,
				AString passphrase,
				AString proxy_host,
				uint16_t proxy_port,
				AString server
			)
			{
				this->key = key;
				this->secret = secret/*.encode()*/;
				this->passphrase = passphrase;

				if (server == "TEST")
				{
					this->simulated = true;
				}

				this->connect_time = time_point_cast<seconds>(system_clock::now()).time_since_epoch().count();

				this->init(REST_HOST, proxy_host, proxy_port);
				this->start();
				this->exchange->write_log("REST API started successfully");

				this->query_time();
				this->query_order();
				this->query_contract();
			}

			void OkxRestApi::query_order()
			{
				Request request{ .method = "GET", .path = "/api/v5/trade/orders-pending" };

				this->async_request<ResJson>(request)
					.done([&](const ResJson& response) {
						on_query_order(response.serialize());
					})
					.fail([&](const Error& error) {
						Printf("fail");
					})
					.error([&](const std::exception& e) {
						Printf("error");
					})
					.send();
			}

			void OkxRestApi::query_time()
			{
				Request request{ .method = "GET", .path = "/api/v5/public/time" };

				this->async_request<ResJson>(request)
					.done([&](const ResJson& response) {
						on_query_time(response.serialize());
					})
					.fail([&](const Error& error) {
						Printf("fail");
					})
					.error([&](const std::exception& e) {
						Printf("error");
					})
					.send();
			}

			void OkxRestApi::query_contract()
			{
				const AStringList inst_types = { "SPOT", "SWAP", "FUTURES"/*, "OPTION"*/ };
				Json args = Json::array();

				for (auto& inst_type : inst_types)
				{
					Request request{ 
						.method = "GET", 
						.path = "/api/v5/public/instruments?instType=" + inst_type 
					};

					this->async_request<ResJson>(request)
						.done([&](const ResJson& response) {
							on_instrument(response.serialize());
						})
						.fail([&](const Error& error) {
								Printf("fail");
						})
						.error([&](const std::exception& e) {
							Printf("error");
						})
						.send();
				}
			}

			void OkxRestApi::on_query_order(const Json& packet)
			{
				for (const Json& order_info : packet["data"])
				{
					OrderData order = parse_order_data(order_info, this->exchange_name);

					this->exchange->on_order(order);
				}

				this->exchange->write_log("Entrustment information query successful");
			}

			void OkxRestApi::on_query_time(const Json& packet)
			{
				AString server_time = DateTimeToString(DateTimeFromStringTime(packet["data"][0]["ts"]));
				AString local_time = DateTimeToString(currentDateTime());
				AString msg = Printf("Server time: %s, local time: %s", server_time.c_str(), local_time.c_str());
				this->exchange->write_log(msg);
			}

			void OkxRestApi::on_instrument(const Json& packet)
			{
				AString instType;
				for (const Json& d : packet["data"])
				{
					AString symbol = d["instId"];
					instType = d["instType"];
					Product product = PRODUCT_OKX2KT[instType];
					bool net_position = true;
					float size;
					if (product == Product::SPOT)
						size = 1;
					else
						size = JsonToFloat(d["ctMult"]);

					ContractData contract{
						.symbol = symbol,
						.exchange = Exchange::OKX,
						.name = symbol,
						.product = product,
						.size = size,
						.pricetick = JsonToFloat(d["tickSz"]),
						.min_volume = JsonToFloat(d["minSz"]),
						.history_data = true,
						.net_position = net_position,
						.exchange_name = this->exchange_name,
					};
					contract.__post_init__();


					//if (product == Product::OPTION)
					//{
					//	auto timestamp = std::atoll(AString(d["expTime"]).c_str());
					//	AString option_expiry = DateTimeToString(DateTimeFromTimestamp(timestamp), "%Y%m%d");

					//	contract.option_strike = float(d["stk"]);
					//	contract.option_type = OPTIONTYPE_OKXO2KT[d["optType"]];
					//	contract.option_expiry = timestamp;
					//	contract.option_portfolio = d["uly"];
					//	contract.option_index = d["stk"];
					//	contract.option_underlying = StringJoin({ contract.option_portfolio, option_expiry }, "_");
					//}

					symbol_contract_map[contract.symbol] = contract;
					this->exchange->on_contract(contract);
				}

				this->exchange->write_log(Printf("%s contract information query successful", instType.c_str()));
			}

			void OkxRestApi::on_error(const std::exception& ex, const Request& request)
			{
				AString msg = Printf("Trigger exception: %s, status code: message: %s\n", ex.what(), request.__str__().c_str());

				fprintf(stderr, "%s", msg.c_str());
			}

			std::list<BarData> OkxRestApi::query_history(const HistoryRequest& req)
			{
				std::multimap <DateTime, BarData> buf;
				AString end_time;

				for (int i = 0; i < 15; ++i)
				{
					AString path = "/api/v5/market/candles";

					Params params = {
						{ "instId", req.symbol },
						{ "bar", INTERVAL_KT2OKX[req.interval] }
					};

					if (!end_time.empty())
					{
						params["after"] = end_time;
					}

					Response resp = this->request("GET", path, params);

					if (resp.status / 100 != 2)
					{
						AString msg = Printf("Failed to obtain historical data, status code: %d, information: %s", resp.status,
							resp.body.c_str());
						this->exchange->write_log(msg);
						break;
					}
					else
					{
						Json data = Json::parse(resp.body); //get() is needed!
						if (!data.contains("data"))
						{
							AString m = data["msg"];
							AString msg = Printf("The historical data obtained is empty, %s", m.c_str());
							break;
						}

						for (auto l : data["data"])
						{
							BarData bar;
							bar.symbol = req.symbol;
							bar.exchange = req.exchange;
							bar.datetime = DateTimeFromStringTime(l[0]);
							bar.interval = req.interval;
							bar.volume = JsonToFloat(l[5]);
							bar.open_price = JsonToFloat(l[1]);
							bar.high_price = JsonToFloat(l[2]);
							bar.low_price = JsonToFloat(l[3]);
							bar.close_price = JsonToFloat(l[4]);
							bar.exchange_name = this->exchange_name;
							bar.__post_init__();

							buf.insert(make_pair(bar.datetime, bar));
						}

						AString begin = (*data["data"].rbegin())[0];
						AString end = (*data["data"].begin())[0];
						AString format_begin = DateTimeToString(DateTimeFromStringTime(begin));
						AString format_end = DateTimeToString(DateTimeFromStringTime(end));

						AString msg = Printf("Successfully obtained historical data, %s - %s, %s - %s",
							req.symbol.c_str(), interval_to_str(req.interval).c_str(),
							format_begin.c_str(), format_end.c_str());
						this->exchange->write_log(msg);

						end_time = begin;
					}
				}

				std::list <BarData> history;
				for (auto iter = buf.begin(); iter != buf.end(); iter = buf.upper_bound(iter->first))
				{
					auto res = buf.equal_range(iter->first);
					for (auto i = res.first; i != res.second; ++i)
					{
						history.push_back(i->second);
					}
				}

				return history;
			}




			OkxWebsocketPublicApi::OkxWebsocketPublicApi(OkxExchange* exchange)
				: WebsocketClient()
			{
				this->exchange = exchange;
				this->exchange_name = exchange->exchange_name;

				this->callbacks = {
					{ "tickers", std::bind(&OkxWebsocketPublicApi::on_ticker, this, _1) },
					{ "books5" , std::bind(&OkxWebsocketPublicApi::on_depth, this, _1) },
				};
			}

			void OkxWebsocketPublicApi::connect(
				AString proxy_host,
				uint16_t proxy_port,
				AString server
			)
			{
				if (server == "REAL")
				{
					this->init(PUBLIC_WEBSOCKET_HOST, proxy_host, proxy_port, 10);
				}
				else
				{
					this->init(TEST_PUBLIC_WEBSOCKET_HOST, proxy_host, proxy_port, 10);
				}

				this->start();
			}

			void OkxWebsocketPublicApi::subscribe(const SubscribeRequest& req)
			{
				this->subscribed[req.kt_symbol] = req;

				TickData tick{
					.symbol = req.symbol,
					.exchange = req.exchange,
					.name = req.symbol,
					.datetime = currentDateTime(),
					.exchange_name = this->exchange_name
				};

				tick.__post_init__();

				this->ticks[req.symbol] = tick;

				const AStringList channels = { "tickers", "books5" };

				Json args = Json::array();

				for (auto& channel : channels)
				{
					args.push_back({ {"channel",channel}, {"instId", req.symbol} });
				}

				Json okx_req = {
					{ "op", "subscribe" },
					{ "args", args }
				};

				this->send_packet(okx_req);
			}

			void OkxWebsocketPublicApi::on_connected()
			{
				this->exchange->write_log("Websocket Public API connection successful");

				for (auto& [key, req] : subscribed)
					this->subscribe(req);
			}

			void OkxWebsocketPublicApi::on_disconnected()
			{
				this->exchange->write_log("Websocket Public API connection disconnected");
			}

			void OkxWebsocketPublicApi::on_packet(const Json& packet)
			{
				if (packet.count("event"))
				{
					const AString& event = packet["event"];
					if (event == "subscribe")
					{
						return;
					}
					else if (event == "error")
					{
						const int& code = packet["code"];
						const AString& msg = packet["message"];
						this->exchange->write_log(Printf("Websocket Public API request exception, status code: %d, information: %s", code, msg.c_str()));
					}
				}
				else
				{
					const AString& channel = packet["arg"]["channel"];
					const Json& data = packet["data"];

					auto callback = this->callbacks.find(channel);
					if (callback != this->callbacks.end())
					{
						callback->second(data);
					}
				}
			}

			void OkxWebsocketPublicApi::on_error(const std::exception& ex)
			{
				AString msg = Printf("Triggered an exception, status code: {exception_type}, message: %s", ex.what());
				this->exchange->write_log(msg);

				fprintf(stderr, "%s", msg.c_str());
			}

			void OkxWebsocketPublicApi::on_ticker(const Json& data)
			{
				for (const Json& d : data)
				{
					const AString& symbol = d["instId"];

					TickData& tick = ticks[symbol];
					tick.last_price = JsonToFloat(d["last"]);
					tick.open_price = JsonToFloat(d["open24h"]);
					tick.high_price = JsonToFloat(d["high24h"]);
					tick.low_price = JsonToFloat(d["low24h"]);
					tick.volume = JsonToFloat(d["vol24h"]);

					this->exchange->on_tick(tick);
				}
			}

			void OkxWebsocketPublicApi::on_depth(const Json& data)
			{
				for (const Json& d : data)
				{
					AString symbol = d["instId"];

					TickData& tick = ticks[symbol];

					const Json& bids = d["bids"];
					const Json& asks = d["asks"];

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

					tick.datetime = DateTimeFromStringTime(d["ts"]);

					this->exchange->on_tick(copy(tick));
				}
			}




			OkxWebsocketPrivateApi::OkxWebsocketPrivateApi(OkxExchange* exchange)
				: WebsocketClient()
			{
				this->exchange = exchange;
				this->exchange_name = exchange->exchange_name;

				this->reqid = 0;
				this->order_count = 0;
				this->connect_time = 0;

				this->callbacks = {
					{ "login", std::bind(&OkxWebsocketPrivateApi::on_login, this, _1) },
					{ "orders" , std::bind(&OkxWebsocketPrivateApi::on_order, this, _1) },
					{ "account", std::bind(&OkxWebsocketPrivateApi::on_account, this, _1) },
					{ "positions", std::bind(&OkxWebsocketPrivateApi::on_position, this, _1) },
					{ "order" , std::bind(&OkxWebsocketPrivateApi::on_send_order, this, _1) },
					{ "cancel-order", std::bind(&OkxWebsocketPrivateApi::on_cancel_order, this, _1) },
					{ "error", std::bind(&OkxWebsocketPrivateApi::on_api_error, this, _1) }
				};
			}

			void OkxWebsocketPrivateApi::connect(
				AString key,
				AString secret,
				AString passphrase,
				AString proxy_host,
				uint16_t proxy_port,
				AString server
			)
			{
				this->connect_time = time_point_cast<seconds>(system_clock::now()).time_since_epoch().count();

				this->key = key;
				this->secret = secret;
				this->passphrase = passphrase;

				if (server == "REAL")
					this->init(PRIVATE_WEBSOCKET_HOST, proxy_host, proxy_port, 10);
				else
					this->init(TEST_PRIVATE_WEBSOCKET_HOST, proxy_host, proxy_port, 10);

				this->start();
			}

			void OkxWebsocketPrivateApi::on_connected()
			{
				this->exchange->write_log("Websocket Private API connection successful");
				this->login();
			}

			void OkxWebsocketPrivateApi::on_disconnected()
			{
				this->exchange->write_log("Websocket Private API connection disconnected");
			}

			void OkxWebsocketPrivateApi::on_packet(const Json& packet)
			{
				AString cb_name;
				if (packet.count("event"))
				{
					cb_name = packet["event"];
				}
				else if (packet.count("op"))
				{
					cb_name = packet["op"];
				}
				else
				{
					cb_name = packet["arg"]["channel"];
				}

				auto callback = this->callbacks.find(cb_name);
				if (callback != this->callbacks.end())
				{
					callback->second(packet);
				}
			}

			void OkxWebsocketPrivateApi::on_error(const std::exception& ex)
			{
				AString msg = Printf("Private channel triggers an exception, status code: {exception_type}, message: %s", ex.what());
				this->exchange->write_log(msg);

				fprintf(stderr, "%s", msg.c_str());
			}

			void OkxWebsocketPrivateApi::on_api_error(const Json& packet)
			{
				AString code = packet["code"];
				AString msg = packet["msg"];
				this->exchange->write_log(Printf("Websocket Private API request failed, status code: %s, message: %s",
					code.c_str(), msg.c_str()));
			}

			void OkxWebsocketPrivateApi::on_login(const Json& packet)
			{
				if (packet["code"] == "0")
				{
					this->exchange->write_log("Websocket Private API login successful");
					this->subscribe_topic();
				}
				else
					this->exchange->write_log("Websocket Private API login failed");
			}

			void OkxWebsocketPrivateApi::on_order(const Json& packet)
			{
				const Json& data = packet["data"];
				for (const Json& d : data)
				{
					OrderData order = parse_order_data(d, this->exchange_name);
					this->exchange->on_order(order);
					if (d["fillSz"] == "0")
						return;

					float trade_volume = JsonToFloat(d["fillSz"]);

					if (symbol_contract_map.count(order.symbol))
					{
						ContractData contract = symbol_contract_map[order.symbol];
						trade_volume = round_to(trade_volume, contract.min_volume);
					}

					TradeData trade{
						.symbol = order.symbol,
						.exchange = order.exchange,
						.orderid = order.orderid,
						.tradeid = d["tradeId"],
						.direction = order.direction,
						.offset = order.offset,
						.price = JsonToFloat(d["fillPx"]),
						.volume = trade_volume,
						.datetime = DateTimeFromStringTime(d["uTime"]),
						.exchange_name = this->exchange_name
					};
					trade.__post_init__();

					this->exchange->on_trade(trade);
				}
			}

			void OkxWebsocketPrivateApi::on_account(const Json& packet)
			{
				if (packet["data"].size() == 0)
					return;

				const Json& buf = packet["data"][0];
				for (const Json& detail : buf["details"])
				{
					AccountData account = AccountData{
						.accountid = detail["ccy"],
						.balance = JsonToFloat(detail["eq"]),
						.exchange_name = this->exchange_name
					};
					account.__post_init__();

					account.available = detail.contains("availEq") ? JsonToFloat(detail["availEq"]) : 0.0;
					account.frozen = account.balance - account.available;

					this->exchange->on_account(account);
				}
			}

			void OkxWebsocketPrivateApi::on_position(const Json& packet)
			{
				const Json& data = packet["data"];
				for (const Json& d : data)
				{
					AString symbol = d["instId"];
					float pos = JsonToFloat(d["pos"]);
					float price = JsonToFloat(d["avgPx"]);
					float pnl = JsonToFloat(d["upl"]);

					PositionData position{
						.symbol = symbol,
						.exchange = Exchange::OKX,
						.direction = Direction::NET,
						.volume = pos,
						.price = price,
						.pnl = pnl,
						.exchange_name = this->exchange_name,
					};
					position.__post_init__();

					this->exchange->on_position(position);
				}
			}

			void OkxWebsocketPrivateApi::on_send_order(const Json& packet)
			{
				const Json& data = packet["data"];

				if (packet["code"] != "0")
				{
					if (data.is_null())
					{
						OrderData& order = this->reqid_order_map[packet["id"]];
						order.status = Status::REJECTED;
						this->exchange->on_order(order);
						return;
					}
				}

				for (const Json& d : data)
				{
					AString code = d["sCode"];
					if (code == "0")
						return;

					AString orderid = d["clOrdId"];
					std::optional<OrderData> order = this->exchange->get_order(orderid);
					if (!order.has_value())
						return;
					order->status = Status::REJECTED;
					this->exchange->on_order(order.value());

					AString msg = d["sMsg"];
					this->exchange->write_log(Printf("Commission failed, status code: %s, message: %s",
						code.c_str(), msg.c_str()));
				}
			}

			void OkxWebsocketPrivateApi::on_cancel_order(const Json& packet)
			{
				if (packet["code"] != "0")
				{
					AString code = packet["code"];
					AString msg = packet["msg"];
					this->exchange->write_log(Printf("Order cancellation failed, status code: %s, message: %s",
						code.c_str(), msg.c_str()));
					return;
				}

				const Json& data = packet["data"];
				for (const Json& d : data)
				{
					AString code = d["sCode"];
					if (code == "0")
						return;

					AString msg = d["sMsg"];
					this->exchange->write_log(Printf("Order cancellation failed, status code: %s, message: %s",
						code.c_str(), msg.c_str()));
				}
			}

			void OkxWebsocketPrivateApi::login()
			{
				auto getTime = []() {
					time_t t;
					time(&t);
					return Printf("%ld", t);
					};

				AString timestamp = getTime();

				AString msg = timestamp + "GET" + "/users/self/verify";
				AString signature = okx_generate_signature(msg, this->secret);

				Json args = {
					{ "apiKey", this->key },
					{ "passphrase", this->passphrase },
					{ "timestamp", timestamp },
					{ "sign", signature }
				};

				Json req = {
					{ "op", "login" },
					{ "args", Json::array({ args })}
				};

				this->send_packet(req);
			}

			void OkxWebsocketPrivateApi::subscribe_topic()
			{
				Json okx_req = {
					{"op", "subscribe"},
					{"args",
						{
							{
								{"channel", "orders"},
								{"instType", "ANY"}
							},
							{
								{"channel", "account"}
							},
							{
								{"channel", "positions"},
								{"instType", "ANY"}
							}
						}
					}
				};

				this->send_packet(okx_req);
			}

			AString OkxWebsocketPrivateApi::send_order(const OrderRequest& req)
			{
				if (!ORDERTYPE_KT2OKX.count(req.type))
				{
					this->exchange->write_log(Printf("Delegation failed, unsupported delegation type: %d", req.type));
					return AString();
				}

				if (symbol_contract_map.find(req.symbol) == symbol_contract_map.end())
				{
					this->exchange->write_log(Printf("Commission failed. The contract code %s could not be found.", req.symbol.c_str()));
					return AString();
				}

				ContractData contract = symbol_contract_map[req.symbol];

				this->order_count += 1;
				AString count_str = std::to_string(this->order_count);
				count_str = Rjust(count_str, 6, '0');

				AString orderid = std::to_string(this->connect_time) + count_str;

				Json args = {
					{ "instId", req.symbol },
					{ "clOrdId", orderid },
					{ "side", DIRECTION_KT2OKX[req.direction] },
					{ "ordType", ORDERTYPE_KT2OKX[req.type] },
					{ "px", std::to_string(req.price) },
					{ "sz", std::to_string(req.volume) }
				};

				if (contract.product == Product::SPOT)
				{
					args["tdMode"] = "cash";
				}
				else
				{
					args["tdMode"] = "isolated";
					args["posSide"] = POSSIZE_KT2OKX[req.direction];
				}

				this->reqid += 1;
				Json okx_req = {
					{ "id", std::to_string(this->reqid) },
					{ "op", "order"  },
					{ "args", {args} }
				};
				this->send_packet(okx_req);

				OrderData order = req.create_order_data(orderid, this->exchange_name);
				this->exchange->on_order(order);
				return order.orderid;
			}

			void OkxWebsocketPrivateApi::cancel_order(const CancelRequest& req)
			{
				Json args = { {"instId", req.symbol} };

				if (local_orderids.count(req.orderid))
					args["clOrdId"] = req.orderid;
				else
					args["ordId"] = req.orderid;

				this->reqid += 1;
				Json okx_req = {
					{ "id", std::to_string(this->reqid) },
					{ "op", "cancel-order" },
					{ "args", {args} }
				};
				this->send_packet(okx_req);
			}



			AString okx_generate_signature(AString msg, AString secret_key)
			{
				AString sign;
				unsigned char* mac = NULL;
				unsigned int mac_length = 0;
				AString key = secret_key;
				HmacEncode("sha256", key.c_str(), key.length(), msg.c_str(), msg.length(), mac, mac_length);
				sign = Base64Encode(AString((char*)mac, mac_length));
				return sign;
			}

			AString generate_timestamp()
			{
				DateTime tp = currentDateTime();

				auto fmt = DateTimeToString(tp);

				return fmt;
			}


			OrderData parse_order_data(const Json& data, AString exchange_name)
			{
				AString order_id = data.value("clOrdId", "");

				if (order_id.empty())
				{
					order_id = data.value("ordId", "");
				}
				else
				{
					local_orderids.insert(order_id);
				}

				OrderData order{
					.symbol = data["instId"],
					.exchange = Exchange::OKX,
					.orderid = order_id,
					.type = ORDERTYPE_OKX2KT[data["ordType"]],
					.direction = DIRECTION_OKX2KT[data["side"]],
					.offset = Offset::NONE,
					.price = JsonToFloat(data["px"]),
					.volume = JsonToFloat(data["sz"]),
					.traded = JsonToFloat(data["accFillSz"]),
					.status = STATUS_OKX2KT[data["state"]],
					.datetime = DateTimeFromStringTime(data["cTime"]),
					.exchange_name = exchange_name,
				};

				order.__post_init__();

				return order;
			}

			float get_float_value(const Json& data, AString key)
			{
				AString data_str = data.value(key, "");
				if (data_str.empty())
					return 0.0;
				return std::atof(data_str.c_str());

			}
		}
	}
}
