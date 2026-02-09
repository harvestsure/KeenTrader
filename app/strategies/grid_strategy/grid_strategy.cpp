#include <api/Globals.h>
#include "grid_strategy.h"
#include <cmath>

#define CLASS_NAME "GridStrategy"

using namespace std::placeholders;


GridStrategy::GridStrategy(CtaEngine* cta_engine,
	AString strategy_name,
	AString kt_symbol,
	Json setting)
	: CtaTemplate(cta_engine, strategy_name, kt_symbol, setting)
{
	parameters = { "grid_upper_price", "grid_lower_price", "grid_count", "grid_volume", "auto_init_orders" };
	variables = { "current_price", "active_buy_count", "active_sell_count" };
}

GridStrategy::~GridStrategy()
{
}

AString GridStrategy::__class__()
{
	return CLASS_NAME;
}

void GridStrategy::on_init()
{
	write_log("Grid Strategy initialization");
	grid_levels.clear();
	active_buy_count = 0;
	active_sell_count = 0;
}

void GridStrategy::on_start()
{
	write_log("Grid Strategy start");
	
	if (grid_upper_price <= 0 || grid_lower_price <= 0 || grid_upper_price <= grid_lower_price)
	{
		write_log("Error: Invalid grid price range");
		return;
	}

	init_grid_levels();

	if (auto_init_orders)
	{
		// Place initial buy and sell orders
		for (auto& level : grid_levels)
		{
			place_buy_order(level);
			place_sell_order(level);
		}
		write_log(Printf("Grid initialized with %lu levels", grid_levels.size()));
	}
}

void GridStrategy::on_stop()
{
	write_log("Grid Strategy stop");
	cancel_all_grid_orders();
}

void GridStrategy::on_bar(const BarData& bar)
{
	current_price = bar.close_price;
}

void GridStrategy::on_tick(const TickData& tick)
{
	current_price = tick.last_price;

	// Find which grid level the current price is at
	int grid_index = find_grid_index_by_price(current_price);
	
	if (grid_index >= 0 && grid_index < static_cast<int>(grid_levels.size()))
	{
		GridLevel& current_level = grid_levels[grid_index];

		// Below current grid: should have buy orders
		for (int i = 0; i < grid_index; i++)
		{
			GridLevel& level = grid_levels[i];
			if (!level.buy_filled && level.buy_orderid.empty())
			{
				place_buy_order(level);
			}
		}

		// Above current grid: should have sell orders
		for (int i = grid_index + 1; i < static_cast<int>(grid_levels.size()); i++)
		{
			GridLevel& level = grid_levels[i];
			if (!level.sell_filled && level.sell_orderid.empty())
			{
				place_sell_order(level);
			}
		}

		// Cancel opposite side orders that are too far away
		for (int i = 0; i < grid_index; i++)
		{
			GridLevel& level = grid_levels[i];
			if (!level.sell_orderid.empty() && !level.sell_filled)
			{
				cancel_order(level.sell_orderid);
				level.sell_orderid = "";
			}
		}

		for (int i = grid_index + 1; i < static_cast<int>(grid_levels.size()); i++)
		{
			GridLevel& level = grid_levels[i];
			if (!level.buy_orderid.empty() && !level.buy_filled)
			{
				cancel_order(level.buy_orderid);
				level.buy_orderid = "";
			}
		}
	}
}

void GridStrategy::on_trade(const TradeData& trade)
{
	int grid_index = find_grid_index_by_price(trade.price);
	if (grid_index >= 0 && grid_index < static_cast<int>(grid_levels.size()))
	{
		GridLevel& level = grid_levels[grid_index];

		if (trade.direction == Direction::LONG && trade.offset == Offset::OPEN)
		{
			level.buy_filled = true;
			write_log(Printf("Grid buy filled at level %d, price=%.4f, volume=%.4f", 
				grid_index, trade.price, trade.volume));
		}
		else if (trade.direction == Direction::SHORT && trade.offset == Offset::CLOSE)
		{
			level.sell_filled = true;
			write_log(Printf("Grid sell filled at level %d, price=%.4f, volume=%.4f", 
				grid_index, trade.price, trade.volume));
		}
	}
}

void GridStrategy::on_order(const OrderData& order)
{
	int grid_index = find_grid_index_by_orderid(order.kt_orderid);
	
	if (grid_index >= 0 && grid_index < static_cast<int>(grid_levels.size()))
	{
		GridLevel& level = grid_levels[grid_index];

		// Update active counts
		if (!order.is_active())
		{
			if (order.direction == Direction::LONG)
			{
				if (!level.buy_orderid.empty())
					active_buy_count--;
			}
			else
			{
				if (!level.sell_orderid.empty())
					active_sell_count--;
			}
		}
	}
}

void GridStrategy::init_grid_levels()
{
	grid_levels.clear();

	float price_diff = grid_upper_price - grid_lower_price;
	float price_step = price_diff / (grid_count - 1);

	for (int i = 0; i < grid_count; i++)
	{
		GridLevel level;
		level.price = grid_lower_price + i * price_step;
		grid_levels.push_back(level);
	}

	write_log(Printf("Grid levels created: %d levels from %.4f to %.4f, step=%.4f",
		grid_count, grid_lower_price, grid_upper_price, price_step));
}

void GridStrategy::cancel_all_grid_orders()
{
	for (auto& level : grid_levels)
	{
		if (!level.buy_orderid.empty())
		{
			cancel_order(level.buy_orderid);
			level.buy_orderid = "";
		}
		if (!level.sell_orderid.empty())
		{
			cancel_order(level.sell_orderid);
			level.sell_orderid = "";
		}
	}
	active_buy_count = 0;
	active_sell_count = 0;
}

void GridStrategy::place_buy_order(GridLevel& level)
{
	if (!level.buy_orderid.empty() || level.buy_filled)
		return;

	AStringList orderids = Buy(level.price, grid_volume);
	if (!orderids.empty())
	{
		level.buy_orderid = orderids.front();
		active_buy_count++;
		write_log(Printf("Place buy order at level price=%.4f, volume=%.4f", 
			level.price, grid_volume));
	}
}

void GridStrategy::place_sell_order(GridLevel& level)
{
	if (!level.sell_orderid.empty() || level.sell_filled)
		return;

	AStringList orderids = Sell(level.price, grid_volume);
	if (!orderids.empty())
	{
		level.sell_orderid = orderids.front();
		active_sell_count++;
		write_log(Printf("Place sell order at level price=%.4f, volume=%.4f", 
			level.price, grid_volume));
	}
}

int GridStrategy::find_grid_index_by_price(float price)
{
	if (grid_levels.empty())
		return -1;

	// Find the closest grid level to the given price
	int best_index = 0;
	float min_diff = std::abs(grid_levels[0].price - price);

	for (int i = 1; i < static_cast<int>(grid_levels.size()); i++)
	{
		float diff = std::abs(grid_levels[i].price - price);
		if (diff < min_diff)
		{
			min_diff = diff;
			best_index = i;
		}
	}

	return best_index;
}

int GridStrategy::find_grid_index_by_orderid(const AString& orderid)
{
	for (int i = 0; i < static_cast<int>(grid_levels.size()); i++)
	{
		if (grid_levels[i].buy_orderid == orderid || grid_levels[i].sell_orderid == orderid)
		{
			return i;
		}
	}
	return -1;
}


extern "C"
{
	KEEN_DECL_EXPORT const char* GetStrategyClass()
	{
		return CLASS_NAME;
	}

	KEEN_DECL_EXPORT CtaTemplate* GetStrategyInstance(CtaEngine* cta_engine, const char* strategy_name, const char* kt_symbol, Json setting)
	{
		return new GridStrategy(cta_engine, strategy_name, kt_symbol, setting);
	}
}
