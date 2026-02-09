#pragma once

#include <app/cta_strategy/template.h>
#include <engine/utility.h>

using namespace Keen::app;

class DcaStrategy : public CtaTemplate
{
public:
	DcaStrategy(CtaEngine* cta_engine,
		AString strategy_name,
		AString kt_symbol,
		Json setting);

	~DcaStrategy();

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
	float invest_amount = 100.0;            // Single investment amount
	float invest_interval = 24.0;           // Investment interval (hours)
	float target_price = 0.0;               // Target sell price, 0 means no sell
	int max_invest_times = 10;              // Maximum number of investments, 0 means unlimited

	// Variables
	int invest_count = 0;                   // Number of investments made
	float average_cost = 0.0;               // Average cost price
	DateTime last_invest_time;              // Last investment time
	float total_invested = 0.0;             // Total invested amount
	int total_shares = 0;                   // Total shares held
	AStringSet active_orderids;             // Set of active order IDs
};


extern "C"
{
	KEEN_DECL_EXPORT const char* GetStrategyClass();
	KEEN_DECL_EXPORT CtaTemplate* GetStrategyInstance(CtaEngine*, const char*, const char*, Json);
}
