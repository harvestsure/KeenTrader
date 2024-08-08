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


protected:

	AString author = "KeKe";

	float setup_coef = 0.25;
	float break_coef = 0.2;
	float enter_coef_1 = 1.07;
	float enter_coef_2 = 0.07;
	float fixed_size = 1;
	float donchian_window = 30;

	float trailing_long = 0.4;
	float trailing_short = 0.4;
	float multiplier = 3;

	float buy_break = 0;
	float sell_setup = 0;
	float sell_enter = 0;
	float buy_enter = 0;
	float buy_setup = 0;
	float sell_break = 0;

	float intra_trade_high = 0;
	float intra_trade_low = 0;

	float day_high = 0;
	float day_open = 0;
	float day_close = 0;
	float day_low = 0;
	float tend_high = 0;
	float tend_low = 0;

	ArrayManager* am;
	Keen::engine::BarGenerator* bg;

	std::deque<BarData> bars;
};



extern "C"
{
	KEEN_DECL_EXPORT const char* GetStrategyClass();
	KEEN_DECL_EXPORT CtaTemplate* GetStrategyInstance(CtaEngine*, const char*, const char*, Json);
}

