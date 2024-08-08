#include <api/Globals.h>
#include "object.h"

namespace Keen
{
	namespace engine
	{
		const std::set<Status> ACTIVE_STATUSES = { Status::SUBMITTING, Status::NOTTRADED, Status::PARTTRADED };

		bool OrderData::is_active() const
		{
			if (ACTIVE_STATUSES.count(this->status))
				return true;
			else
				return false;
		}

		CancelRequest OrderData::create_cancel_request()
		{
			CancelRequest req{
				.orderid = this->orderid,
				.symbol = this->symbol,
				.exchange = this->exchange
			};
			req.__post_init__();

			return req;
		}

		OrderData OrderRequest::create_order_data(AString orderid, AString exchange_name) const
		{
			OrderData order{
				.symbol = this->symbol,
				.exchange = this->exchange,
				.orderid = orderid,
				.type = this->type,
				.direction = this->direction,
				.offset = this->offset,
				.price = this->price,
				.volume = this->volume,
				.reference = this->reference,
				.exchange_name = exchange_name
			};
			order.__post_init__();

			return order;
		}

		bool QuoteData::is_active() const
		{
			if (ACTIVE_STATUSES.count(this->status))
				return true;
			else
				return false;
		}

		CancelRequest QuoteData::create_cancel_request()
		{
			CancelRequest req{
			.orderid = this->quoteid,
			.symbol = this->symbol,
			.exchange = this->exchange
			};
			req.__post_init__();

			return req;
		}

		QuoteData QuoteRequest::create_quote_data(AString quoteid, AString exchange_name) const
		{
			QuoteData quote{
				.symbol = this->symbol,
				.exchange = this->exchange,
				.quoteid = quoteid,
				.bid_price = this->bid_price,
				.bid_volume = this->bid_volume,
				.ask_price = this->ask_price,
				.ask_volume = this->ask_volume,
				.bid_offset = this->bid_offset,
				.ask_offset = this->ask_offset,
				.reference = this->reference,
				.exchange_name = exchange_name
			};
			quote.__post_init__();

			return quote;
		}
	}
}
