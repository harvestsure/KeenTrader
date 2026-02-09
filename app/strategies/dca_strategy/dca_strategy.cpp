#include <api/Globals.h>
#include "dca_strategy.h"

#define CLASS_NAME "DcaStrategy"

using namespace std::placeholders;


DcaStrategy::DcaStrategy(CtaEngine* cta_engine,
	AString strategy_name,
	AString kt_symbol,
	Json setting)
	: CtaTemplate(cta_engine, strategy_name, kt_symbol, setting)
{
	parameters = { "invest_amount", "invest_interval", "target_price", "max_invest_times" };
	variables = { "invest_count", "average_cost", "total_invested", "total_shares" };
}

DcaStrategy::~DcaStrategy()
{
}

AString DcaStrategy::__class__()
{
	return CLASS_NAME;
}

void DcaStrategy::on_init()
{
	write_log("DCA Strategy initialization");
	invest_count = 0;
	average_cost = 0.0;
	total_invested = 0.0;
	total_shares = 0;
}

void DcaStrategy::on_start()
{
	write_log("DCA Strategy start");
	last_invest_time = currentDateTime();
}

void DcaStrategy::on_stop()
{
	write_log("DCA Strategy stop");
	cancel_all();
}

void DcaStrategy::on_bar(const BarData& bar)
{
	// Check if it's time to invest
	if (last_invest_time.time_since_epoch().count() == 0)
	{
		last_invest_time = bar.datetime;
	}

	// Check max invest times
	if (max_invest_times > 0 && invest_count >= max_invest_times)
	{
		return;
	}

	// Calculate hours passed since last investment
	auto duration = bar.datetime - last_invest_time;
	auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();

	if (hours >= invest_interval)
	{
		// Calculate investment volume based on current close price
		if (bar.close_price > 0)
		{
			float volume = invest_amount / bar.close_price;
			
			// Send buy order
			AStringList orderids = Buy(bar.close_price, volume);
			for (auto& oid : orderids)
			{
				active_orderids.insert(oid);
			}

			last_invest_time = bar.datetime;
			invest_count++;
			total_invested += invest_amount;

			write_log(Printf("DCA invest #%d: amount=%.2f, price=%.2f, volume=%.4f",
				invest_count, invest_amount, bar.close_price, volume));
		}
	}

	// Check if need to sell at target price
	if (target_price > 0 && bar.close_price >= target_price && pos > 0)
	{
		AStringList orderids = Sell(bar.close_price, pos);
		for (auto& oid : orderids)
		{
			active_orderids.insert(oid);
		}

		write_log(Printf("DCA target sell: price=%.2f, volume=%.4f", bar.close_price, pos));
	}
}

void DcaStrategy::on_tick(const TickData& tick)
{
	// Tick level logic can be added here if needed
}

void DcaStrategy::on_trade(const TradeData& trade)
{
	// Update average cost when buying
	if (trade.direction == Direction::LONG && trade.offset == Offset::OPEN)
	{
		total_shares += static_cast<int>(trade.volume);
		average_cost = (average_cost * (total_shares - static_cast<int>(trade.volume)) + trade.price * trade.volume) / total_shares;

		write_log(Printf("Trade filled - avg_cost: %.4f, total_shares: %d", average_cost, total_shares));
	}
}

void DcaStrategy::on_order(const OrderData& order)
{
	// Remove order from active set if it's no longer active
	if (!order.is_active() && active_orderids.count(order.kt_orderid))
	{
		active_orderids.erase(order.kt_orderid);
	}

	// Log order status changes
	write_log(Printf("Order status: %s, status=%d", order.kt_orderid.c_str(), static_cast<int>(order.status)));
}


extern "C"
{
	KEEN_DECL_EXPORT const char* GetStrategyClass()
	{
		return CLASS_NAME;
	}

	KEEN_DECL_EXPORT CtaTemplate* GetStrategyInstance(CtaEngine* cta_engine, const char* strategy_name, const char* kt_symbol, Json setting)
	{
		return new DcaStrategy(cta_engine, strategy_name, kt_symbol, setting);
	}
}
