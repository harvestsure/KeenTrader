#include <api/Globals.h>
#include "converter.h"
#include "engine/engine.h"


namespace Keen
{
	namespace engine
	{
		OffsetConverter::OffsetConverter(TradeEngine* trade_engine) : trade_engine(trade_engine) {}

		void OffsetConverter::update_position(const PositionData& position) {
			if (!is_convert_required(position.kt_symbol)) {
				return;
			}

			auto holding = get_position_holding(position.kt_symbol);
			holding->update_position(position);
		}

		void OffsetConverter::update_trade(const TradeData& trade) {
			if (!is_convert_required(trade.kt_symbol)) {
				return;
			}

			auto holding = get_position_holding(trade.kt_symbol);
			holding->update_trade(trade);
		}

		void OffsetConverter::update_order(const OrderData& order) {
			if (!is_convert_required(order.kt_symbol)) {
				return;
			}

			auto holding = get_position_holding(order.kt_symbol);
			holding->update_order(order);
		}

		void OffsetConverter::update_order_request(const OrderRequest& req, const std::string& kt_orderid) {
			if (!is_convert_required(req.kt_symbol)) {
				return;
			}

			auto holding = get_position_holding(req.kt_symbol);
			holding->update_order_request(req, kt_orderid);
		}

		std::shared_ptr<PositionHolding> OffsetConverter::get_position_holding(const std::string& kt_symbol) {
			auto holding = holdings.find(kt_symbol);
			if (holding == holdings.end()) {
				auto contract = trade_engine->get_contract(kt_symbol);
				if (contract) {
					holding = holdings.emplace(kt_symbol, std::make_shared<PositionHolding>(*contract)).first;
				}
			}
			return holding->second;
		}

		std::list<OrderRequest> OffsetConverter::convert_order_request(const OrderRequest& req, bool lock, bool net) {
			if (!is_convert_required(req.kt_symbol)) {
				return { req };
			}

			auto holding = get_position_holding(req.kt_symbol);

			if (lock) {
				return holding->convert_order_request_lock(req);
			}
			else if (net) {
				return holding->convert_order_request_net(req);
			}
			else if (req.exchange == Exchange::SHFE || req.exchange == Exchange::INE) {
				return holding->convert_order_request_shfe(req);
			}
			else {
				return { req };
			}
		}

		bool OffsetConverter::is_convert_required(const std::string& kt_symbol) {
			auto contract = trade_engine->get_contract(kt_symbol);

			if (!contract || contract->net_position) {
				return false;
			}
			else {
				return true;
			}
		}



		
        PositionHolding::PositionHolding(const ContractData& contract)
            : kt_symbol(contract.kt_symbol), exchange(contract.exchange),
            long_pos(0), long_yd(0), long_td(0),
            short_pos(0), short_yd(0), short_td(0),
            long_pos_frozen(0), long_yd_frozen(0), long_td_frozen(0),
            short_pos_frozen(0), short_yd_frozen(0), short_td_frozen(0) {}

        void PositionHolding::update_position(const PositionData& position) {
            if (position.direction == Direction::LONG) {
                long_pos = position.volume;
                long_yd = position.yd_volume;
                long_td = long_pos - long_yd;
            }
            else {
                short_pos = position.volume;
                short_yd = position.yd_volume;
                short_td = short_pos - short_yd;
            }
        }

        void PositionHolding::update_order(const OrderData& order) {
            if (order.is_active()) {
                active_orders[order.kt_orderid] = std::make_shared<OrderData>(order);
            }
            else {
                active_orders.erase(order.kt_orderid);
            }
            calculate_frozen();
        }

        void PositionHolding::update_order_request(const OrderRequest& req, const std::string& kt_orderid) {
            auto exchange_name = kt_orderid.substr(0, kt_orderid.find("."));
            auto orderid = kt_orderid.substr(kt_orderid.find(".") + 1);
            OrderData order = req.create_order_data(orderid, exchange_name);
            update_order(order);
        }

        void PositionHolding::update_trade(const TradeData& trade) {
            if (trade.direction == Direction::LONG) {
                if (trade.offset == Offset::OPEN) {
                    long_td += trade.volume;
                }
                else if (trade.offset == Offset::CLOSETODAY) {
                    short_td -= trade.volume;
                }
                else if (trade.offset == Offset::CLOSEYESTERDAY) {
                    short_yd -= trade.volume;
                }
                else if (trade.offset == Offset::CLOSE) {
                    if (trade.exchange == Exchange::SHFE || trade.exchange == Exchange::INE) {
                        short_yd -= trade.volume;
                    }
                    else {
                        short_td -= trade.volume;
                        if (short_td < 0) {
                            short_yd += short_td;
                            short_td = 0;
                        }
                    }
                }
            }
            else {
                if (trade.offset == Offset::OPEN) {
                    short_td += trade.volume;
                }
                else if (trade.offset == Offset::CLOSETODAY) {
                    long_td -= trade.volume;
                }
                else if (trade.offset == Offset::CLOSEYESTERDAY) {
                    long_yd -= trade.volume;
                }
                else if (trade.offset == Offset::CLOSE) {
                    if (trade.exchange == Exchange::SHFE || trade.exchange == Exchange::INE) {
                        long_yd -= trade.volume;
                    }
                    else {
                        long_td -= trade.volume;
                        if (long_td < 0) {
                            long_yd += long_td;
                            long_td = 0;
                        }
                    }
                }
            }

            long_pos = long_td + long_yd;
            short_pos = short_td + short_yd;
            sum_pos_frozen();
        }

        void PositionHolding::calculate_frozen() {
            long_pos_frozen = 0;
            long_yd_frozen = 0;
            long_td_frozen = 0;

            short_pos_frozen = 0;
            short_yd_frozen = 0;
            short_td_frozen = 0;

            for (const auto& order_pair : active_orders) {
                const auto& order = order_pair.second;
                if (order->offset == Offset::OPEN) {
                    continue;
                }

                double frozen = order->volume - order->traded;
                if (order->direction == Direction::LONG) {
                    if (order->offset == Offset::CLOSETODAY) {
                        short_td_frozen += frozen;
                    }
                    else if (order->offset == Offset::CLOSEYESTERDAY) {
                        short_yd_frozen += frozen;
                    }
                    else if (order->offset == Offset::CLOSE) {
                        short_td_frozen += frozen;
                        if (short_td_frozen > short_td) {
                            short_yd_frozen += (short_td_frozen - short_td);
                            short_td_frozen = short_td;
                        }
                    }
                }
                else if (order->direction == Direction::SHORT) {
                    if (order->offset == Offset::CLOSETODAY) {
                        long_td_frozen += frozen;
                    }
                    else if (order->offset == Offset::CLOSEYESTERDAY) {
                        long_yd_frozen += frozen;
                    }
                    else if (order->offset == Offset::CLOSE) {
                        long_td_frozen += frozen;
                        if (long_td_frozen > long_td) {
                            long_yd_frozen += (long_td_frozen - long_td);
                            long_td_frozen = long_td;
                        }
                    }
                }
            }
            sum_pos_frozen();
        }

        void PositionHolding::sum_pos_frozen() {
            long_td_frozen = std::min(long_td_frozen, long_td);
            long_yd_frozen = std::min(long_yd_frozen, long_yd);

            short_td_frozen = std::min(short_td_frozen, short_td);
            short_yd_frozen = std::min(short_yd_frozen, short_yd);

            long_pos_frozen = long_td_frozen + long_yd_frozen;
            short_pos_frozen = short_td_frozen + short_yd_frozen;
        }

        std::list<OrderRequest> PositionHolding::convert_order_request_shfe(const OrderRequest& req) {
            if (req.offset == Offset::OPEN) {
                return { req };
            }

            int pos_available = req.direction == Direction::LONG ? short_pos - short_pos_frozen : long_pos - long_pos_frozen;
            int td_available = req.direction == Direction::LONG ? short_td - short_td_frozen : long_td - long_td_frozen;

            if (req.volume > pos_available) {
                return {};
            }
            else if (req.volume <= td_available) {
                OrderRequest req_td = req;
                req_td.offset = Offset::CLOSETODAY;
                return { req_td };
            }
            else {
                std::list<OrderRequest> req_list;

                if (td_available > 0) {
                    OrderRequest req_td = req;
                    req_td.offset = Offset::CLOSETODAY;
                    req_td.volume = td_available;
                    req_list.push_back(req_td);
                }

                OrderRequest req_yd = req;
                req_yd.offset = Offset::CLOSEYESTERDAY;
                req_yd.volume = req.volume - td_available;
                req_list.push_back(req_yd);

                return req_list;
            }
        }

        std::list<OrderRequest> PositionHolding::convert_order_request_lock(const OrderRequest& req) {
            float td_volume = req.direction == Direction::LONG ? short_td : long_td;
            float yd_available = req.direction == Direction::LONG ? short_yd - short_yd_frozen : long_yd - long_yd_frozen;
            std::set<Exchange> close_yd_exchanges = { Exchange::SHFE, Exchange::INE };

            if (td_volume && close_yd_exchanges.find(exchange) == close_yd_exchanges.end()) {
                OrderRequest req_open = req;
                req_open.offset = Offset::OPEN;
                return { req_open };
            }
            else {
                int close_volume = std::min(req.volume, yd_available);
                int open_volume = std::max(0.f, req.volume - yd_available);
                std::list<OrderRequest> req_list;

                if (yd_available > 0) {
                    OrderRequest req_yd = req;
                    req_yd.offset = close_yd_exchanges.contains(exchange) ? Offset::CLOSEYESTERDAY : Offset::CLOSE;
                    req_yd.volume = close_volume;
                    req_list.push_back(req_yd);
                }

                if (open_volume > 0) {
                    OrderRequest req_open = req;
                    req_open.offset = Offset::OPEN;
                    req_open.volume = open_volume;
                    req_list.push_back(req_open);
                }

                return req_list;
            }
        }

        std::list<OrderRequest> PositionHolding::convert_order_request_net(const OrderRequest& req) {
            float pos_available = req.direction == Direction::LONG ? short_pos - short_pos_frozen : long_pos - long_pos_frozen;
            float td_available = req.direction == Direction::LONG ? short_td - short_td_frozen : long_td - long_td_frozen;
            float yd_available = req.direction == Direction::LONG ? short_yd - short_yd_frozen : long_yd - long_yd_frozen;

            std::list<OrderRequest> reqs;
            float volume_left = req.volume;

            if (req.exchange == Exchange::SHFE || req.exchange == Exchange::INE) {
                if (td_available > 0) {
                    int td_volume = std::min(td_available, volume_left);
                    volume_left -= td_volume;

                    OrderRequest td_req = req;
                    td_req.offset = Offset::CLOSETODAY;
                    td_req.volume = td_volume;
                    reqs.push_back(td_req);
                }

                if (yd_available > 0 && volume_left > 0) {
                    int yd_volume = std::min(yd_available, volume_left);
                    volume_left -= yd_volume;

                    OrderRequest yd_req = req;
                    yd_req.offset = Offset::CLOSEYESTERDAY;
                    yd_req.volume = yd_volume;
                    reqs.push_back(yd_req);
                }
            }
            else {
                if (pos_available > 0) {
                    int pos_volume = std::min(pos_available, volume_left);
                    volume_left -= pos_volume;

                    OrderRequest pos_req = req;
                    pos_req.offset = Offset::CLOSE;
                    pos_req.volume = pos_volume;
                    reqs.push_back(pos_req);
                }
            }

            if (volume_left > 0) {
                OrderRequest open_req = req;
                open_req.offset = Offset::OPEN;
                open_req.volume = volume_left;
                reqs.push_back(open_req);
            }

            return reqs;
        }
	}
}
