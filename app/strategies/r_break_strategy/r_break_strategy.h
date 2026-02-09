#pragma once

#include <app/cta_strategy/template.h>
#include <engine/utility.h>

using namespace Keen::app;

class ArrayManager;

class RBreakStrategy : public CtaTemplate
{
public:
	RBreakStrategy(CtaEngine* cta_engine,
		AString strategy_name,
		AString kt_symbol,
		Json setting);

	~RBreakStrategy();

	virtual AString __class__();

	virtual void on_init();

	virtual void on_start();

	virtual void on_stop();

	virtual void on_bar(const BarData& bar);

	virtual void on_tick(const TickData& tick);

	virtual void on_trade(const TradeData& trade);

	virtual void on_order(const OrderData& order);

protected:

	AString author = "KeenTrader";

	// Classic R-Break Parameters
	int donchian_window = 20;                // Donchian channel period (days)
	float setup_ratio = 0.35;                // Setup line ratio
	float break_ratio = 0.07;                // Break line ratio
	float position_size = 1.0;               // Order size per trade
	float trailing_stop_pct = 0.5;           // Trailing stop percentage

	// Strategy Variables
	float buy_setup = 0;                     // Long setup line (entry on breakout)
	float sell_setup = 0;                    // Short setup line (entry on breakdown)
	float buy_break = 0;                     // Long stop-loss line
	float sell_break = 0;                    // Short stop-loss line

	float hh = 0;                            // Highest price over N days
	float ll = 0;                            // Lowest price over N days
	float range = 0;                         // Range over N days (hh - ll)

	float intra_trade_high = 0;              // Intraday high since entry
	float intra_trade_low = 0;               // Intraday low since entry

	AStringSet active_orderids;              // Set of active order IDs

	ArrayManager* am;
	Keen::engine::BarGenerator* bg;

	std::deque<BarData> bars;
	
	// Helper methods
	void calculate_lines();
	void place_orders(const BarData& bar);
};



extern "C"
{
	KEEN_DECL_EXPORT const char* GetStrategyClass();
	KEEN_DECL_EXPORT CtaTemplate* GetStrategyInstance(CtaEngine*, const char*, const char*, Json);
}