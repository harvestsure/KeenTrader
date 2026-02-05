#include <api/Globals.h>
#include <engine/engine.h>
#include <engine/utility.h>
#include "okx_exchange.h"
#include "algo_hmac.h"
#include "utils.h"

using namespace std::placeholders;

#define BUFLEN 65536

namespace Keen
{
	namespace exchange
	{
		namespace okx
		{
			// Real server hosts
			const AString REAL_REST_HOST = "https://www.okx.com";
			const AString REAL_PUBLIC_HOST = "wss://ws.okx.com:8443/ws/v5/public";
			const AString REAL_PRIVATE_HOST = "wss://ws.okx.com:8443/ws/v5/private";
			const AString REAL_BUSINESS_HOST = "wss://ws.okx.com:8443/ws/v5/business";

			// AWS server hosts
			const AString AWS_REST_HOST = "https://aws.okx.com";
			const AString AWS_PUBLIC_HOST = "wss://wsaws.okx.com:8443/ws/v5/public";
			const AString AWS_PRIVATE_HOST = "wss://wsaws.okx.com:8443/ws/v5/private";
			const AString AWS_BUSINESS_HOST = "wss://wsaws.okx.com:8443/ws/v5/business";

			// Demo server hosts
			const AString DEMO_REST_HOST = "https://www.okx.com";
			const AString DEMO_PUBLIC_HOST = "wss://wspap.okx.com:8443/ws/v5/public?brokerId=9999";
			const AString DEMO_PRIVATE_HOST = "wss://wspap.okx.com:8443/ws/v5/private?brokerId=9999";
			const AString DEMO_BUSINESS_HOST = "wss://wspap.okx.com:8443/ws/v5/business?brokerId=9999";

			AString okx_generate_signature(AString msg, AString secret_key);

			AString generate_timestamp();

			static std::map<AString, Status> STATUS_OKX2KT = {
				{"live", Status::NOTTRADED},
				{"partially_filled", Status::PARTTRADED},
				{"filled", Status::ALLTRADED},
				{"canceled", Status::CANCELLED},
				{"mmp_canceled", Status::CANCELLED} };

			static std::map<AString, OrderType> ORDERTYPE_OKX2KT = {
				{"limit", OrderType::LIMIT},
				{"market", OrderType::MARKET},
				{"fok", OrderType::FOK},
				{"ioc", OrderType::FAK} };

			static std::map<OrderType, AString> ORDERTYPE_KT2OKX = SwapMap(ORDERTYPE_OKX2KT);

			static std::map<AString, Direction> DIRECTION_OKX2KT = {
				{"buy", Direction::LONG},
				{"sell", Direction::SHORT} };

			static std::map<Direction, AString> DIRECTION_KT2OKX = SwapMap(DIRECTION_OKX2KT);

			static std::map<Interval, AString> INTERVAL_KT2OKX = {
				{Interval::MINUTE, "1m"},
				{Interval::HOUR, "1H"},
				{Interval::DAILY, "1D"} };

			static std::map<AString, Product> PRODUCT_OKX2KT = {
				{"SWAP", Product::SWAP},
				{"SPOT", Product::SPOT},
				{"FUTURES", Product::FUTURES} };

			static std::map<Product, AString> PRODUCT_KT2OKX = SwapMap(PRODUCT_OKX2KT);

			float get_float_value(const Json& data, AString key);

			std::pair<AString, AString> get_side_pos(Direction direction, Offset offset, bool dualSide);

			static bool g_dualSide = true; // dual-side mode


			OkxExchange::OkxExchange(EventEmitter* event_emitter)
				: CryptoExchange(event_emitter, "OKX")
			{
				default_setting =
				{
					{"api_key", ""},
					{"secret_key", ""},
					{"passphrase", ""},
					{"proxy_host", ""},
					{"proxy_port", 0},
					{"server", ""} //["REAL", "AWS", "DEMO"]
				};

				exchange = Exchange::OKX;

				this->rest_api = new OkxRestApi(this);
				this->public_api = new OkxWebsocketPublicApi(this);
				this->private_api = new OkxWebsocketPrivateApi(this);
			}

			OkxExchange::~OkxExchange()
			{
				SAFE_RELEASE(rest_api);
				SAFE_RELEASE(public_api);
				SAFE_RELEASE(private_api);
			}

			void OkxExchange::connect(const Json& setting)
			{
				this->key = setting.value("api_key", "");
				this->secret = setting.value("secret_key", "");
				this->passphrase = setting.value("passphrase", "");

				this->proxy_host = setting.value("proxy_host", "");
				this->proxy_port = setting.value("proxy_port", 0);
				this->server = setting.value("server", "");
				this->hedge_mode = setting.value("hedge_mode", false);

				this->rest_api->connect(
					this->key,
					this->secret,
					this->passphrase,
					this->server,
					this->hedge_mode,
					this->proxy_host,
					this->proxy_port);
			}

			void OkxExchange::connect_ws_api()
			{
				this->public_api->connect(
					this->server,
					this->proxy_host,
					this->proxy_port);

				this->private_api->connect(
					this->key,
					this->secret,
					this->passphrase,
					this->server,
					this->proxy_host,
					this->proxy_port);
			}

			void OkxExchange::subscribe(const SubscribeRequest& req)
			{
				this->public_api->subscribe(req);
			}

			AString OkxExchange::send_order(const OrderRequest& req)
			{
				return this->private_api->send_order(req);
			}

			void OkxExchange::cancel_order(const CancelRequest& req)
			{
				this->private_api->cancel_order(req);
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
				this->public_api->stop();
				this->private_api->stop();
			}

			bool OkxExchange::set_position_mode(PositionMode mode)
			{
				if (!CryptoExchange::set_position_mode(mode))
				{
					return false;
				}

				this->rest_api->set_position_mode(mode);
				
				return true;
			}

			bool OkxExchange::set_leverage(const AString& symbol, int leverage, const Json& params)
			{
				if (!CryptoExchange::set_leverage(symbol, leverage, params))
				{
					return false;
				}

				this->rest_api->set_leverage(symbol, leverage);
				
				return true;
			}

			void OkxExchange::on_order(const OrderData& order)
			{
				this->orders[order.orderid] = order;
				BaseExchange::on_order(order);
			}

			std::optional<OrderData> OkxExchange::get_order(const AString& orderid)
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

			void OkxExchange::on_contract(const ContractData& contract)
			{
				this->symbol_contract_map[contract.symbol] = contract;
				this->name_contract_map[contract.name] = contract;

				BaseExchange::on_contract(contract);
			}

			std::optional<ContractData> OkxExchange::get_contract_by_symbol(const AString& symbol)
			{
				return GetWithNone(this->symbol_contract_map, symbol);
			}

			std::optional<ContractData> OkxExchange::get_contract_by_name(const AString& name)
			{
				return GetWithNone(this->name_contract_map, name);
			}

			OrderData OkxExchange::parse_order_data(const Json& data, const AString& gateway_name)
			{
				auto contract = this->get_contract_by_name(data["instId"]);

				AString order_id = data.value("clOrdId", "");

				if (order_id.empty())
				{
					order_id = data.value("ordId", "");
				}
				else
				{
					this->local_orderids.insert(order_id);
				}

				OrderData order{
					.symbol = contract->symbol,
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
					path = request.path + '?' + /*URLEncode */ BuildParams(request.params);
				else
					path = request.path;

				AString msg = timestamp + request.method + path + request_data;
				AString signature = okx_generate_signature(msg, this->secret);

				// Add headers
				request.headers = {
					{"OK-ACCESS-KEY", this->key},
					{"OK-ACCESS-SIGN", signature},
					{"OK-ACCESS-TIMESTAMP", timestamp},
					{"OK-ACCESS-PASSPHRASE", this->passphrase},
					{"Content-Type", "application/json"} };

				if (this->simulated)
					request.headers.insert(std::pair("x-simulated-trading", "1"));

				return request;
			}

			void OkxRestApi::connect(
				AString key,
				AString secret,
				AString passphrase,
				AString server,
				bool hedge_mode,
				AString proxy_host,
				uint16_t proxy_port)
			{
				this->key = key;
				this->secret = secret /*.encode()*/;
				this->passphrase = passphrase;
				this->hedge_mode = hedge_mode;

				if (server == "DEMO")
				{
					this->simulated = true;
				}

				this->connect_time = time_point_cast<seconds>(system_clock::now()).time_since_epoch().count();

				static const std::unordered_map<AString, AString> server_hosts = {
					{"REAL", REAL_REST_HOST},
					{"AWS", AWS_REST_HOST},
					{"DEMO", DEMO_REST_HOST},
				};

				const AString& host = server_hosts.at(server);

				this->init(host, proxy_host, proxy_port);
				this->start();
				this->exchange->write_log("REST API started");

				this->query_time();
				this->set_position_mode(this->hedge_mode ? PositionMode::HEDGE : PositionMode::ONE_WAY);
				this->query_contract();
			}

			void OkxRestApi::query_order()
			{
				Request request{
					.method = "GET",
					.path = "/api/v5/trade/orders-pending",
					.callback = std::bind(&OkxRestApi::on_query_order, this, _1, _2) };

				this->request(request);
			}

			void OkxRestApi::query_time()
			{
				Request request{
					.method = "GET",
					.path = "/api/v5/public/time",
					.callback = std::bind(&OkxRestApi::on_query_time, this, _1, _2) };

				this->request(request);
			}

			void OkxRestApi::query_contract()
			{
				Json args = Json::array();

				for (auto& [inst_type, type] : PRODUCT_OKX2KT)
				{
					Request request{
						.method = "GET",
						.path = "/api/v5/public/instruments?instType=" + inst_type,
						.callback = std::bind(&OkxRestApi::on_query_contract, this, _1, _2) };

					this->request(request);
				}
			}

			void OkxRestApi::set_position_mode(PositionMode mode)
			{
				AString mode_str = (mode == PositionMode::HEDGE) ? "long_short_mode" : "net_mode";

				Json body = {
					{"posMode", mode_str}
				};
				Request request{
					.method = "POST",
					.path = "/api/v5/account/set-position-mode",
					.data = body.dump(),
					.callback = std::bind(&OkxRestApi::on_set_position_mode, this, _1, _2),
					.on_failed = [this](const Error& error, const Request& request)
                    {
                        const AString errorData = error.errorData();
                        // if (errorData.find("\"code\":-4059") != AString::npos)
                        // {
                        //     this->exchange->write_log("No need to change position side. It is already set.");
                        // }
                        // else
                        {
                            const AString msg = Printf("Set position mode failed, status code: %d, information: %s",
                                error.status(), errorData.c_str());

                            this->exchange->write_log(msg);
                        }
                    }
				};
				this->request(request);
			}

			void OkxRestApi::set_leverage(const AString& symbol, int leverage)
			{
				Json body = {
					{"instId", symbol},
					{"lever", std::to_string(leverage)},
					{"mgnMode", "cross"}
				};
				Request request{
					.method = "POST",
					.path = "/api/v5/account/set-leverage",
					.data = body.dump(),
					.callback = std::bind(&OkxRestApi::on_set_leverage, this, _1, _2)
				};
				this->request(request);
			}

			void OkxRestApi::on_query_order(const Json& packet, const Request& request)
			{
				for (const Json& order_info : packet["data"])
				{
					const OrderData& order = this->exchange->parse_order_data(order_info, this->exchange_name);

					this->exchange->on_order(order);
				}

				this->exchange->write_log("Entrustment information query successful");
			}

			void OkxRestApi::on_query_time(const Json& packet, const Request& request)
			{
				AString server_time = DateTimeToString(DateTimeFromStringTime(packet["data"][0]["ts"]));
				AString local_time = DateTimeToString(currentDateTime(), true);
				AString msg = Printf("server time: %s, local time: %s", server_time.c_str(), local_time.c_str());
				this->exchange->write_log(msg);
			}

			void OkxRestApi::on_query_contract(const Json& packet, const Request& request)
			{
				AString instType;
				for (const Json& d : packet["data"])
				{
					AString name = d["instId"];
					instType = d["instType"];
					Product product = PRODUCT_OKX2KT[instType];
					bool net_position = true;
					float size;
					if (product == Product::SPOT)
						size = 1;
					else
						size = JsonToFloat(d["ctMult"]);

					//std::string symbol;

					//switch (product)
					//{
					//case Product::SPOT:
					//{
					//	ReplaceString(name, "-", "");
					//	symbol = name + "_SPOT_OKX";
					//}
					//break;
					//case Product::SWAP:
					//{
					//	auto parts = StringSplit(name, "-");
					//	symbol = parts[0] + parts[1] + "_SWAP_OKX";
					//}
					//break;
					//case Product::FUTURES:
					//{
					//	auto parts = StringSplit(name, "-");
					//	symbol = parts[0] + parts[1] + "_" + parts[2] + "_OKX";
					//}
					//break;
					//default:
					//	break;
					//}

					ContractData contract{
						.symbol = name,
						.exchange = Exchange::OKX,
						.name = name,
						.product = product,
						.size = size,
						.pricetick = JsonToFloat(d["tickSz"]),
						.min_volume = JsonToFloat(d["minSz"]),
						.history_data = true,
						.net_position = net_position,
						.exchange_name = this->exchange_name,
					};
					contract.__post_init__();

					this->exchange->on_contract(contract);

					this->product_ready.insert(contract.product);
				}

				this->exchange->write_log(Printf("%s contract information query successful", instType.c_str()));

				if (this->product_ready.size() == PRODUCT_OKX2KT.size())
				{
					this->query_order();

					this->exchange->connect_ws_api();
				}
			}

			void OkxRestApi::on_set_position_mode(const Json& packet, const Request& request)
			{
				if (packet.contains("code"))
				{
					AString code = packet["code"];
					if (code == "0")
					{
						this->exchange->write_log("Set position mode succeeded");
					}
					else
					{
						AString msg = packet.value("msg", "Unknown error");
						this->exchange->write_log("Set position mode failed: " + msg);
					}
				}
			}

			void OkxRestApi::on_set_leverage(const Json& packet, const Request& request)
			{
				if (packet.contains("code"))
				{
					AString code = packet["code"];
					if (code == "0" && packet.contains("data") && packet["data"].is_array() && packet["data"].size() > 0)
					{
						const Json& data = packet["data"][0];
						AString symbol = data.value("instId", "");
						int leverage = std::stoi(data.value("lever", "1"));
						AString msg = Printf("为 %s 设置杠杆成功: %d 倍", symbol.c_str(), leverage);
						this->exchange->write_log(msg);
					}
					else
					{
						AString msg = packet.value("msg", "未知错误");
						this->exchange->write_log("设置杠杆失败: " + msg);
					}
				}
			}

			void OkxRestApi::on_error(const std::exception& ex, const Request& request)
			{
				AString msg = Printf("Trigger exception: %s, status code: message: %s\n", ex.what(), request.__str__().c_str());

				fprintf(stderr, "%s", msg.c_str());
			}

			std::list<BarData> OkxRestApi::query_history(const HistoryRequest& req)
			{
				auto contract = this->exchange->get_contract_by_symbol(req.symbol);
				if (!contract)
				{
					this->exchange->write_log(Printf("Query kline history failed, symbol not found: %s", req.symbol.c_str()));
					return {};
				}

				std::list<BarData> history;
				AString after = std::to_string(duration_cast<milliseconds>(req.end.time_since_epoch()).count());
				int limit = 100;
				bool has_error = false;

				while (!has_error)
				{
					AString path = "/api/v5/market/candles";

					Params params = {
						{ "instId", req.symbol },
						{ "limit", std::to_string(limit) },
						{ "bar", INTERVAL_KT2OKX[req.interval]},
						{ "after", after },
					};

					Response resp = this->request("GET", path, params);

					if (resp.status / 100 != 2)
					{
						AString msg = Printf("Query kline history failed, status code: %d, information: %s",
							resp.status, resp.body.c_str());
						this->exchange->write_log(msg);
						has_error = true;
						break;
					}
					else
					{
						try {
							Json data = Json::parse(resp.body); // get() is needed!
							if (!data.contains("data"))
							{
								AString m = data["msg"];
								AString msg = Printf("The historical data obtained is empty, %s", m.c_str());
								has_error = true;
								break;
							}

							const Json& bar_data = data.value("data", Json::array());

							if (!bar_data.size())
								break;

							for (auto row : bar_data)
							{
								if (!row.is_array() || row.size() < 6) {
									continue;
								}

								BarData bar;
								bar.symbol = req.symbol;
								bar.exchange = req.exchange;
								bar.datetime = DateTimeFromStringTime(row[0]);
								bar.interval = req.interval;
								bar.volume = JsonToFloat(row[5]);
								bar.open_price = JsonToFloat(row[1]);
								bar.high_price = JsonToFloat(row[2]);
								bar.low_price = JsonToFloat(row[3]);
								bar.close_price = JsonToFloat(row[4]);
								bar.exchange_name = this->exchange_name;
								bar.__post_init__();

								history.push_back(bar);
							}

							AString begin = (*bar_data.rbegin())[0];
							AString end = (*bar_data.begin())[0];
							DateTime begin_dt = DateTimeFromStringTime(begin);
							DateTime end_dt = DateTimeFromStringTime(end);


							AString msg = Printf("Query kline history finished, %s - %s, %s - %s",
								req.symbol.c_str(), interval_to_str(req.interval).c_str(),
								DateTimeToString(begin_dt).c_str(), DateTimeToString(end_dt).c_str());
							this->exchange->write_log(msg);

							// Break if all bars have been queried
							if (begin_dt <= req.start)
								break;

							// Update start time
							after = begin;
						}
						catch (const std::exception& e) {
							this->exchange->write_log(Printf("JSON parsing failed: %s", e.what()));
							has_error = true;
							break;
						}

					}
				}

				history.sort([](const BarData& a, const BarData& b) {
					return a.datetime < b.datetime;
					});

				return history;
			}

			OkxWebsocketPublicApi::OkxWebsocketPublicApi(OkxExchange* exchange)
				: WebsocketClient()
			{
				this->exchange = exchange;
				this->exchange_name = exchange->exchange_name;

				this->callbacks = {
					{"tickers", std::bind(&OkxWebsocketPublicApi::on_ticker, this, _1)},
					{"books5", std::bind(&OkxWebsocketPublicApi::on_depth, this, _1)},
				};
			}

			void OkxWebsocketPublicApi::connect(
				AString server,
				AString proxy_host,
				uint16_t proxy_port)
			{

				static const std::unordered_map<AString, AString> server_hosts = {
					{"REAL", REAL_PUBLIC_HOST},
					{"AWS", AWS_PUBLIC_HOST},
					{"DEMO", DEMO_PUBLIC_HOST},
				};

				const AString& host = server_hosts.at(server);

				this->init(host, proxy_host, proxy_port, 20);

				this->start();
			}

			void OkxWebsocketPublicApi::subscribe(const SubscribeRequest& req)
			{
				auto contract = this->exchange->get_contract_by_symbol(req.symbol);
				if (!contract)
				{
					this->exchange->write_log(Printf("Failed to subscribe data, symbol not found: %s", req.symbol.c_str()));
					return;
				}

				this->subscribed[req.symbol] = req;

				TickData tick{
					.symbol = req.symbol,
					.exchange = req.exchange,
					.name = contract->name,
					.datetime = currentDateTime(),
					.exchange_name = this->exchange_name };

				tick.__post_init__();

				this->ticks[req.symbol] = tick;

				const AStringList channels = { "tickers", "books5" };

				Json args = Json::array();

				for (auto& channel : channels)
				{
					args.push_back({ {"channel", channel}, {"instId", contract->name} });
				}

				Json okx_req = {
					{"op", "subscribe"},
					{"args", args} };

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
					float volume = JsonToFloat(d["vol24h"]);

					TickData& tick = ticks[symbol];
					tick.last_price = JsonToFloat(d["last"]);
					tick.open_price = JsonToFloat(d["open24h"]);
					tick.high_price = JsonToFloat(d["high24h"]);
					tick.low_price = JsonToFloat(d["low24h"]);
					tick.last_volume = tick.volume > 0.f ? volume - tick.volume : 0.f;
					tick.volume = volume;
					tick.turnover = JsonToFloat(d["volCcy24h"]);

					tick.datetime = DateTimeFromStringTime(d["ts"]);

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
				: WebsocketClient(), local_orderids(exchange->local_orderids)
			{
				this->exchange = exchange;
				this->exchange_name = exchange->exchange_name;

				this->reqid = 0;
				this->order_count = 0;
				this->connect_time = 0;

				this->callbacks = {
					{"login", std::bind(&OkxWebsocketPrivateApi::on_login, this, _1)},
					{"orders", std::bind(&OkxWebsocketPrivateApi::on_order, this, _1)},
					{"account", std::bind(&OkxWebsocketPrivateApi::on_account, this, _1)},
					{"positions", std::bind(&OkxWebsocketPrivateApi::on_position, this, _1)},
					{"order", std::bind(&OkxWebsocketPrivateApi::on_send_order, this, _1)},
					{"cancel-order", std::bind(&OkxWebsocketPrivateApi::on_cancel_order, this, _1)},
					{"error", std::bind(&OkxWebsocketPrivateApi::on_api_error, this, _1)} };
			}

			void OkxWebsocketPrivateApi::connect(
				AString key,
				AString secret,
				AString passphrase,
				AString server,
				AString proxy_host,
				uint16_t proxy_port)
			{
				this->connect_time = time_point_cast<seconds>(system_clock::now()).time_since_epoch().count();

				this->key = key;
				this->secret = secret;
				this->passphrase = passphrase;

				static const std::unordered_map<AString, AString> server_hosts = {
					{"REAL", REAL_PRIVATE_HOST},
					{"AWS", AWS_PRIVATE_HOST},
					{"DEMO", DEMO_PRIVATE_HOST},
				};

				const AString& host = server_hosts.at(server);

				this->init(host, proxy_host, proxy_port, 20);

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
					const OrderData& order = this->exchange->parse_order_data(d, this->exchange_name);
					this->exchange->on_order(order);

					if (d["fillSz"] == "0")
						return;

					float trade_volume = JsonToFloat(d["fillSz"]);
					auto contract = this->exchange->get_contract_by_symbol(order.symbol);

					if (!contract)
						trade_volume = round_to(trade_volume, contract->min_volume);

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
						.exchange_name = this->exchange_name };
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
						.exchange_name = this->exchange_name };
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
					AString name = d["instId"];
					auto contract = this->exchange->get_contract_by_name(name);

					float pos = JsonToFloat(d["pos"]);
					float price = JsonToFloat(d["avgPx"]);
					float pnl = JsonToFloat(d["upl"]);

					PositionData position{
						.symbol = contract->symbol,
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
					const AString& code = d["sCode"];
					if (code == "0")
						return;

					const AString& orderid = d["clOrdId"];
					std::optional<OrderData> order = this->exchange->get_order(orderid);
					if (!order.has_value())
						return;
					order->status = Status::REJECTED;
					this->exchange->on_order(order.value());

					const AString& msg = d["sMsg"];
					this->exchange->write_log(Printf("Commission failed, status code: %s, message: %s",
						code.c_str(), msg.c_str()));
				}
			}

			void OkxWebsocketPrivateApi::on_cancel_order(const Json& packet)
			{
				if (packet["code"] != "0")
				{
					const AString& code = packet["code"];
					const AString& msg = packet["msg"];
					this->exchange->write_log(Printf("Order cancellation failed, status code: %s, message: %s",
						code.c_str(), msg.c_str()));
					return;
				}

				const Json& data = packet["data"];
				for (const Json& d : data)
				{
					const AString& code = d["sCode"];
					if (code == "0")
						return;

					const AString& msg = d["sMsg"];
					this->exchange->write_log(Printf("Order cancellation failed, status code: %s, message: %s",
						code.c_str(), msg.c_str()));
				}
			}

			void OkxWebsocketPrivateApi::login()
			{
				auto getTime = []()
					{
						time_t t;
						time(&t);
						return Printf("%ld", t);
					};

				AString timestamp = getTime();

				AString msg = timestamp + "GET" + "/users/self/verify";
				AString signature = okx_generate_signature(msg, this->secret);

				Json args = {
					{"apiKey", this->key},
					{"passphrase", this->passphrase},
					{"timestamp", timestamp},
					{"sign", signature} };

				Json req = {
					{"op", "login"},
					{"args", Json::array({args})} };

				this->send_packet(req);
			}

			void OkxWebsocketPrivateApi::subscribe_topic()
			{
				Json okx_req = {
					{"op", "subscribe"},
					{ "args",
						{
							{ {"channel", "orders"}, {"instType", "ANY"} },
							{ {"channel", "account"} },
							{ {"channel", "positions"}, {"instType", "ANY"} }
						}
					}
				};

				this->send_packet(okx_req);
			}

			AString OkxWebsocketPrivateApi::send_order(const OrderRequest& req)
			{
				if (!ORDERTYPE_KT2OKX.count(req.type))
				{
					this->exchange->write_log(Printf("Send order failed, order type not supported: %d", req.type));
					return AString();
				}

				auto contract = this->exchange->get_contract_by_symbol(req.symbol);
				if (!contract)
				{
					this->exchange->write_log(Printf("Send order failed, symbol not found: %s", req.symbol.c_str()));
					return AString();
				}

				this->order_count += 1;
				AString count_str = std::to_string(this->order_count);
				count_str = Rjust(count_str, 6, '0');

				AString orderid = std::to_string(this->connect_time) + count_str;

			// 根据持仓模式决定是否为双向模式
			bool is_hedge_mode = (this->exchange->get_position_mode() == PositionMode::HEDGE);
			auto [side, posSide] = get_side_pos(req.direction, req.offset, is_hedge_mode);

			Json args = {
				{"instId", contract->name},
				{"clOrdId", orderid},
				{"side", side},
				{"posSide", posSide},
				{"ordType", ORDERTYPE_KT2OKX[req.type]},
				{"px", std::to_string(req.price)},
				{"sz", std::to_string(req.volume)} };

				if (contract->product == Product::SPOT)
				{
					args["tdMode"] = "cash";
				}
				else
				{
					args["tdMode"] = "cross";
				}

				this->reqid += 1;
				Json okx_req = {
					{"id", std::to_string(this->reqid)},
					{"op", "order"},
					{"args", {args}}
				};
				this->send_packet(okx_req);

				OrderData order = req.create_order_data(orderid, this->exchange_name);
				this->exchange->on_order(order);
				return order.kt_orderid;
			}

			void OkxWebsocketPrivateApi::cancel_order(const CancelRequest& req)
			{
				auto contract = this->exchange->get_contract_by_symbol(req.symbol);
				if (!contract)
				{
					this->exchange->write_log(Printf("Cancel order failed, symbol not found: %s", req.symbol.c_str()));
					return;
				}

				Json args = { {"instId", contract->name} };

				if (this->local_orderids.count(req.orderid))
					args["clOrdId"] = req.orderid;
				else
					args["ordId"] = req.orderid;

				this->reqid += 1;
				Json okx_req = {
					{"id", std::to_string(this->reqid)},
					{"op", "cancel-order"},
					{"args", {args}} };
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

			float get_float_value(const Json& data, AString key)
			{
				AString data_str = data.value(key, "");
				if (data_str.empty())
					return 0.0;
				return std::atof(data_str.c_str());
			}

			std::pair<AString, AString> get_side_pos(Direction direction, Offset offset, bool dualSide)
			{
				AString side = DIRECTION_KT2OKX[direction];
				AString posSide = "net";

				if (direction == Direction::LONG)
				{
					if (dualSide)
					{
						if (offset == Offset::OPEN)
						{
							posSide = "long";
						}
						else if (offset == Offset::CLOSE)
						{
							posSide = "short";
						}
					}
				}
				else if (direction == Direction::SHORT)
				{
					if (dualSide)
					{
						if (offset == Offset::OPEN)
						{
							posSide = "short";
						}
						else if (offset == Offset::CLOSE)
						{
							posSide = "long";
						}
					}
				}

				return make_pair(side, posSide);
			}
		}
	}
}
