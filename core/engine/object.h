#pragma once

#include "engine/constant.h"

namespace Keen
{
	namespace engine
	{
		class CancelRequest;

		class KEEN_ENGINE_EXPORT TickData
		{
		public:
			AString symbol;
			Exchange exchange;
			AString name;
			DateTime datetime;
			AString exchange_name;

			float volume = 0;
			float turnover = 0;
			float open_interest = 0;
			float last_price = 0;
			float last_volume = 0;
			float limit_up = 0;
			float limit_down = 0;

			float open_price = 0;
			float high_price = 0;
			float low_price = 0;
			float pre_close = 0;

			float bid_price_1 = 0;
			float bid_price_2 = 0;
			float bid_price_3 = 0;
			float bid_price_4 = 0;
			float bid_price_5 = 0;

			float ask_price_1 = 0;
			float ask_price_2 = 0;
			float ask_price_3 = 0;
			float ask_price_4 = 0;
			float ask_price_5 = 0;

			float bid_volume_1 = 0;
			float bid_volume_2 = 0;
			float bid_volume_3 = 0;
			float bid_volume_4 = 0;
			float bid_volume_5 = 0;

			float ask_volume_1 = 0;
			float ask_volume_2 = 0;
			float ask_volume_3 = 0;
			float ask_volume_4 = 0;
			float ask_volume_5 = 0;

			AString kt_symbol;

			void __post_init__()
			{
				this->kt_symbol = this->symbol + "." + exchange_to_str(this->exchange);
			}
		};

		class KEEN_ENGINE_EXPORT BarData
		{
		public:
			AString exchange_name;
			AString symbol;
			Exchange exchange;
			DateTime datetime;

			Interval interval = Interval::None;
			float volume = 0;
			float open_interest = 0;
			float open_price = 0;
			float high_price = 0;
			float low_price = 0;
			float close_price = 0;

			AString kt_symbol;

			void __post_init__()
			{
				this->kt_symbol = this->symbol + "." + exchange_to_str(this->exchange);
			}
		};

		class KEEN_ENGINE_EXPORT CancelRequest
		{
		public:
			AString orderid;
			AString symbol;
			Exchange exchange;

			AString kt_symbol;

			void __post_init__()
			{
				this->kt_symbol = this->symbol + "." + exchange_to_str(this->exchange);
			}
		};

		class KEEN_ENGINE_EXPORT OrderData
		{
		public:
			AString symbol;
			Exchange exchange;
			AString orderid;

			OrderType type = OrderType::LIMIT;
			Direction direction;
			Offset offset = Offset::NONE;
			float price = 0;
			float volume = 0;
			float traded = 0;
			Status status = Status::SUBMITTING;
			DateTime datetime;
			AString reference = "";

			AString exchange_name;

			AString kt_symbol;
			AString kt_orderid;

			bool is_active() const;

			CancelRequest create_cancel_request();

			void __post_init__()
			{
				this->kt_symbol = this->symbol + "." + exchange_to_str(this->exchange);
				this->kt_orderid = this->exchange_name + "." + this->orderid;
			}
		};

		class KEEN_ENGINE_EXPORT TradeData
		{
		public:
			AString symbol;
			Exchange exchange;
			AString orderid;
			AString tradeid;
			Direction direction;

			Offset offset = Offset::NONE;
			float price = 0;
			float volume = 0;
			DateTime datetime;

			AString exchange_name;

			AString kt_symbol;
			AString kt_orderid;
			AString kt_tradeid;

			void __post_init__()
			{
				this->kt_symbol = this->symbol + "." + exchange_to_str(this->exchange);
				this->kt_orderid = this->exchange_name + "." + this->orderid;
				this->kt_tradeid = this->exchange_name + "." + this->tradeid;
			}
		};

		class KEEN_ENGINE_EXPORT PositionData
		{
		public:
			AString symbol;
			Exchange exchange;
			Direction direction;

			float volume = 0;
			float frozen = 0;
			float price = 0;
			float pnl = 0;
			float yd_volume = 0;

			AString exchange_name;

			AString kt_symbol;
			AString kt_positionid;

			void __post_init__()
			{
				this->kt_symbol = this->symbol + "." + exchange_to_str(this->exchange);
				this->kt_positionid = this->kt_symbol + "." + str_direction(this->direction);
			}
		};

		class KEEN_ENGINE_EXPORT AccountData
		{
		public:
			AString accountid;

			float balance = 0;
			float frozen = 0;

			float available;

			AString exchange_name;

			AString kt_accountid;

			void __post_init__()
			{
				this->available = this->balance - this->frozen;
				this->kt_accountid = this->exchange_name + "." + this->accountid;
			}
		};


		class KEEN_ENGINE_EXPORT LogData
		{
		public:
			AString msg;

			int level = /*INFO*/0;

			DateTime time;

			AString exchange_name;
		};


		class KEEN_ENGINE_EXPORT ContractData
		{
		public:
			AString symbol;
			Exchange exchange;
			AString name;
			Product product;
			float size;
			float pricetick;

			float min_volume = 1;           // minimum trading volume of the contract
			float max_volume = 0;
			bool stop_supported = false;    // whether server supports stop order		
			bool history_data = false;      // whether exchange provides bar history data
			bool net_position = false;      // whether exchange uses net position volume

			float option_strike = 0;
			AString option_underlying;     // kt_symbol of underlying contract
			OptionType option_type = OptionType::None;
			uint64_t option_listed = 0;
			uint64_t option_expiry = 0;
			AString option_portfolio;
			AString option_index;

			AString exchange_name;

			AString kt_symbol;

			void __post_init__()
			{
				this->kt_symbol = this->symbol + "." + exchange_to_str(this->exchange);
			}
		};

		class QuoteData
		{
		public:

			AString symbol;
			Exchange exchange;
			AString quoteid;

			float bid_price = 0.0;
			int bid_volume = 0;
			float ask_price = 0.0;
			int ask_volume = 0;
			Offset bid_offset = Offset::NONE;
			Offset ask_offset = Offset::NONE;
			Status status = Status::SUBMITTING;
			DateTime datetime;
			AString reference = "";

			AString exchange_name;

			AString kt_symbol;
			AString kt_quoteid;

			void __post_init__()
			{
				this->kt_symbol = this->symbol + "." + exchange_to_str(this->exchange);
				this->kt_quoteid = this->exchange_name + "." + this->kt_quoteid;
			}

			bool is_active() const;

			CancelRequest create_cancel_request();
		};


		class KEEN_ENGINE_EXPORT SubscribeRequest
		{
		public:
			AString symbol;
			Exchange exchange;

			AString kt_symbol;

			void __post_init__()
			{
				this->kt_symbol = this->symbol + "." + exchange_to_str(this->exchange);
			}
		};


		class KEEN_ENGINE_EXPORT OrderRequest
		{
		public:
			AString symbol;
			Exchange exchange;
			Direction direction;
			OrderType type;
			float volume = 0;
			float price = 0;
			Offset offset = Offset::NONE;

			AString reference;

			AString exchange_name;

			AString kt_symbol;

			void __post_init__()
			{
				this->kt_symbol = this->symbol + "." + exchange_to_str(this->exchange);
			}

			OrderData create_order_data(AString orderid, AString exchange_name) const;
		};


		class KEEN_ENGINE_EXPORT HistoryRequest
		{
		public:
			AString symbol;
			Exchange exchange;
			DateTime start;// datetime
			DateTime end;  // datetime = None
			Interval interval = Interval::None;

			AString kt_symbol;

			void __post_init__()
			{
				this->kt_symbol = this->symbol + "." + exchange_to_str(this->exchange);
			}
		};

		class QuoteRequest
		{
		public:
			AString symbol;
			Exchange exchange;
			float bid_price;
			int bid_volume;
			float ask_price;
			int ask_volume;
			Offset bid_offset = Offset::NONE;
			Offset ask_offset = Offset::NONE;
			AString reference = "";

			AString kt_symbol;

			void __post_init__()
			{
				this->kt_symbol = this->symbol + "." + exchange_to_str(this->exchange);
			}

			QuoteData create_quote_data(AString quoteid, AString exchange_name) const;
		};


		struct EmailMessage
		{
			AString subject;
			AString content;
			AString receiver;
		};

		struct NoticeMessage
		{
			AString subject;
			AString content;
		};
	}
}
