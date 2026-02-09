
#include <api/Globals.h>
#include "r_break_strategy.h"
#include "ArrayManager.h"

#define CLASS_NAME "RBreakStrategy"

using namespace std::placeholders;


RBreakStrategy::RBreakStrategy(CtaEngine* cta_engine,
	AString strategy_name,
	AString kt_symbol,
	Json setting)
	: CtaTemplate(cta_engine, strategy_name, kt_symbol, setting)
	, am(nullptr)
	, bg(nullptr)
{
	parameters = { "donchian_window", "setup_ratio", "break_ratio", "position_size", "trailing_stop_pct" };
	variables = { "buy_setup", "sell_setup", "buy_break", "sell_break", "hh", "ll", "range" };

	am = new ArrayManager();
	bg = new BarGenerator(std::bind(&RBreakStrategy::on_bar, this, _1));
}

RBreakStrategy::~RBreakStrategy()
{
	if (am)
		delete am;
	if (bg)
		delete bg;
}

AString RBreakStrategy::__class__()
{
	return CLASS_NAME;
}

void RBreakStrategy::on_init()
{
	write_log("R-Break Strategy initialized");
	
	buy_setup = 0;
	sell_setup = 0;
	buy_break = 0;
	sell_break = 0;
	hh = 0;
	ll = 0;
	range = 0;
	intra_trade_high = 0;
	intra_trade_low = 0;
}

void RBreakStrategy::on_start()
{
	write_log("R-Break Strategy started");
	load_bar(static_cast<float>(donchian_window) * 1.5, Interval::DAILY);
}

void RBreakStrategy::on_stop()
{
	write_log("R-Break Strategy stopped");
	cancel_all();
}

void RBreakStrategy::calculate_lines()
{
	// Get highest and lowest price in the donchian window
	auto result = am->donchian(donchian_window);
	hh = std::get<0>(result);
	ll = std::get<1>(result);
	
	if (hh <= 0 || ll <= 0 || hh == ll)
		return;

	// Calculate range
	range = hh - ll;

	// Calculate Setup lines (breakout points)
	// When price breaks above sell_setup -> enter long; when it breaks below buy_setup -> enter short
	sell_setup = hh - setup_ratio * range;		// Bullish breakout point
	buy_setup = ll + setup_ratio * range;		// Bearish breakout point

	// Calculate Break lines (stop-loss points)
	// Stop-loss for long positions at buy_break; for short positions at sell_break
	buy_break = ll + break_ratio * range;		// Long stop-loss
	sell_break = hh - break_ratio * range;		// Short stop-loss
}

void RBreakStrategy::place_orders(const BarData& bar)
{
	// No position: Check for entry signal
	if (pos == 0)
	{
		intra_trade_high = bar.high_price;
		intra_trade_low = bar.low_price;

		// Long signal: price breaks above sell_setup
		if (bar.close_price > sell_setup && sell_setup > 0)
		{
			AStringList orderids = Buy(bar.close_price, position_size);
			for (const auto& oid : orderids)
			{
				active_orderids.insert(oid);
			}
			write_log(Printf("LONG signal: close=%.4f > sell_setup=%.4f", bar.close_price, sell_setup));
		}

		// Short signal: price breaks below buy_setup
		else if (bar.close_price < buy_setup && buy_setup > 0)
		{
			AStringList orderids = Short(bar.close_price, position_size);
			for (const auto& oid : orderids)
			{
				active_orderids.insert(oid);
			}
			write_log(Printf("SHORT signal: close=%.4f < buy_setup=%.4f", bar.close_price, buy_setup));
		}
	}

	// Long position: Trailing stop loss
	else if (pos > 0)
	{
		intra_trade_high = std::max(intra_trade_high, bar.high_price);
		float stop_loss = intra_trade_high * (1.0 - trailing_stop_pct / 100.0);

		// Close if price touches stop loss
		if (bar.low_price < stop_loss)
		{
			AStringList orderids = Sell(bar.close_price, pos);
			for (const auto& oid : orderids)
			{
				active_orderids.insert(oid);
			}
			write_log(Printf("LONG exit: trailing stop triggered at %.4f, intra_high=%.4f", stop_loss, intra_trade_high));
		}

		// Also close if price breaks back below buy_break
		else if (bar.close_price < buy_break)
		{
			AStringList orderids = Sell(bar.close_price, pos);
			for (const auto& oid : orderids)
			{
				active_orderids.insert(oid);
			}
			write_log(Printf("LONG exit: break support at buy_break=%.4f", buy_break));
		}
	}

	// Short position: Trailing stop loss
	else if (pos < 0)
	{
		intra_trade_low = std::min(intra_trade_low, bar.low_price);
		float stop_loss = intra_trade_low * (1.0 + trailing_stop_pct / 100.0);

		// Close if price touches stop loss
		if (bar.high_price > stop_loss)
		{
			AStringList orderids = Cover(bar.close_price, -pos);
			for (const auto& oid : orderids)
			{
				active_orderids.insert(oid);
			}
			write_log(Printf("SHORT exit: trailing stop triggered at %.4f, intra_low=%.4f", stop_loss, intra_trade_low));
		}

		// Also close if price breaks back above sell_break
		else if (bar.close_price > sell_break)
		{
			AStringList orderids = Cover(bar.close_price, -pos);
			for (const auto& oid : orderids)
			{
				active_orderids.insert(oid);
			}
			write_log(Printf("SHORT exit: break resistance at sell_break=%.4f", sell_break));
		}
	}
}

void RBreakStrategy::on_bar(const BarData& bar)
{
	// Update array manager with new bar
	if (!am)
		return;

	am->update_bar(bar);
	if (!am->inited())
		return;

	// Keep a small history for track
	bars.push_back(bar);
	if (bars.size() > 100)
		bars.pop_front();

	// Calculate the setup/break lines based on donchian channel
	calculate_lines();

	if (buy_setup <= 0 || sell_setup <= 0)
		return;

	// Execute trading logic
	place_orders(bar);

	// Update event for UI display
	put_event();
}

void RBreakStrategy::on_tick(const TickData& tick)
{
	// Convert tick to bar for minute bar tracking
	if (bg)
		bg->update_tick(tick);
}

void RBreakStrategy::on_trade(const TradeData& trade)
{
	write_log(Printf("Trade executed: %s %s %.4f @ %.4f",
		trade.direction == Direction::LONG ? "BUY" : "SELL",
		trade.symbol.c_str(),
		trade.volume,
		trade.price));
}

void RBreakStrategy::on_order(const OrderData& order)
{
	// Track order status changes
	if (!order.is_active() && active_orderids.count(order.kt_orderid))
	{
		active_orderids.erase(order.kt_orderid);
	}
}

const char* GetStrategyClass()
{
	return CLASS_NAME;
}

CtaTemplate* GetStrategyInstance(CtaEngine* cta_engine, const char* strategy_name, const char* kt_symbol, Json setting)
{
	return new RBreakStrategy(cta_engine, strategy_name, kt_symbol, setting);
}
