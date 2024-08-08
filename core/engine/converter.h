#pragma once

#include "engine/object.h"

namespace Keen
{
	namespace engine
	{
		class TradeEngine;

		class PositionHolding;

		class KEEN_TRADER_EXPORT OffsetConverter
		{
		public:
			OffsetConverter(TradeEngine* trade_engine);

			void update_position(const PositionData& position);
			void update_trade(const TradeData& trade);
			void update_order(const OrderData& order);
			void update_order_request(const OrderRequest& req, const std::string& kt_orderid);
			std::shared_ptr<PositionHolding> get_position_holding(const std::string& kt_symbol);
			std::list<OrderRequest> convert_order_request(const OrderRequest& req, bool lock, bool net = false);
			bool is_convert_required(const std::string& kt_symbol);

		private:
			TradeEngine* trade_engine;
			std::unordered_map<AString, std::shared_ptr<PositionHolding>> holdings;
		};



		class PositionHolding
		{
		public:
			PositionHolding(const ContractData& contract);

			void update_position(const PositionData& position);
			void update_order(const OrderData& order);
			void update_order_request(const OrderRequest& req, const std::string& kt_orderid);
			void update_trade(const TradeData& trade);

			std::list<OrderRequest> convert_order_request_shfe(const OrderRequest& req);
			std::list<OrderRequest> convert_order_request_lock(const OrderRequest& req);
			std::list<OrderRequest> convert_order_request_net(const OrderRequest& req);

		private:
			void calculate_frozen();
			void sum_pos_frozen();

		private:
			std::string kt_symbol;
			Exchange exchange;

			std::unordered_map<std::string, std::shared_ptr<OrderData>> active_orders;

			float long_pos = 0;
			float long_yd = 0;
			float long_td = 0;

			float short_pos = 0;
			float short_yd = 0;
			float short_td = 0;

			float long_pos_frozen = 0;
			float long_yd_frozen = 0;
			float long_td_frozen = 0;

			float short_pos_frozen = 0;
			float short_yd_frozen = 0;
			float short_td_frozen = 0;
		};
	}
}
