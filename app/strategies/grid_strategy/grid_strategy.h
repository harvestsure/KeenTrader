#pragma once

#include <app/cta_strategy/template.h>
#include <engine/utility.h>
#include <vector>
#include <map>

using namespace Keen::app;

// Grid level information
struct GridLevel
{
	float price;			// Grid price level
	bool buy_filled = false;   // Buy order filled at this level
	bool sell_filled = false;  // Sell order filled at this level
	AString buy_orderid = "";  // Buy order ID at this level
	AString sell_orderid = "";  // Sell order ID at this level
};

class GridStrategy : public CtaTemplate
{
public:
	GridStrategy(CtaEngine* cta_engine,
		AString strategy_name,
		AString kt_symbol,
		Json setting);

	~GridStrategy();

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

	// Parameters
	float grid_upper_price = 0.0;			// Grid upper price
	float grid_lower_price = 0.0;			// Grid lower price
	int grid_count = 10;					// Number of grid levels
	float grid_volume = 1.0;				// Volume per grid level
	bool auto_init_orders = true;			// Auto initialize grid orders on start

	// Variables
	std::vector<GridLevel> grid_levels;		// Grid level array
	float current_price = 0.0;				// Current market price
	int active_buy_count = 0;				// Active buy orders count
	int active_sell_count = 0;				// Active sell orders count
	
	// Helper methods
	void init_grid_levels();
	void cancel_all_grid_orders();
	void place_buy_order(GridLevel& level);
	void place_sell_order(GridLevel& level);
	int find_grid_index_by_price(float price);
	int find_grid_index_by_orderid(const AString& orderid);
};


extern "C"
{
	KEEN_DECL_EXPORT const char* GetStrategyClass();
	KEEN_DECL_EXPORT CtaTemplate* GetStrategyInstance(CtaEngine*, const char*, const char*, Json);
}
