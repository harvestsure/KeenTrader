
#include <api/Globals.h>
#include "r_break_strategy.h"
#include "ArrayManager.h"
#include<date/date.h>

#define CLASS_NAME "RBreakStrategy"

using namespace std::chrono;
using namespace std::placeholders;


DateTime getDays(const DateTime& datetime)
{
	return date::floor<date::days>(datetime);
}


RBreakStrategy::RBreakStrategy(CtaEngine* cta_engine,
	AString strategy_name,
	AString kt_symbol,
	Json setting)
	: CtaTemplate(cta_engine, strategy_name, kt_symbol, setting)
	, bg(nullptr)
{
	parameters = { "setup_coef", "break_coef", "enter_coef_1", "enter_coef_2", "fixed_size", "donchian_window" };
	variables = { "buy_break", "sell_setup", "sell_enter", "buy_enter", "buy_setup", "sell_break" };

	am = new ArrayManager();
	bg = new BarGenerator(std::bind(&RBreakStrategy::on_bar, this, _1));
}

RBreakStrategy::~RBreakStrategy()
{

}

AString RBreakStrategy::__class__()
{
	return CLASS_NAME;
}

void RBreakStrategy::on_init()
{
	write_log("Strategy initialization");
}

void RBreakStrategy::on_start()
{
	write_log("Strategy launch");

	load_bar(1, Interval::MINUTE);
}

void RBreakStrategy::on_bar(const BarData& bar)
{
	this->write_log(Printf("on_bar %s %s O:%.2f H:%.2f L:%.2f C:%.2f",
		DateTimeToString(bar.datetime).c_str(),
		bar.symbol.c_str(),
		bar.open_price,
		bar.high_price,
		bar.low_price,
		bar.close_price));
		
	this->cancel_all();

	auto am = this->am;
	am->update_bar(bar);
	if (!am->inited())
		return;

	this->bars.push_back(bar);
	if (bars.size() <= 2)
		return;
	else
		this->bars.pop_front();
	const BarData& last_bar = *(++this->bars.rbegin());

	// New Day
	if (getDays(last_bar.datetime) != getDays(bar.datetime))
	{
		if (this->day_open)
		{
			this->buy_setup = this->day_low - this->setup_coef * (this->day_high - this->day_close);
			this->sell_setup = this->day_high + this->setup_coef * (this->day_close - this->day_low);

			this->buy_enter = (this->enter_coef_1 / 2) * (this->day_high + this->day_low) - this->enter_coef_2 * this->day_high;
			this->sell_enter = (this->enter_coef_1 / 2) * (this->day_high + this->day_low) - this->enter_coef_2 * this->day_low;

			this->buy_break = this->sell_setup + this->break_coef * (this->sell_setup - this->buy_setup);
			this->sell_break = this->buy_setup - this->break_coef * (this->sell_setup - this->buy_setup);
		}

		this->day_open = bar.open_price;
		this->day_high = bar.high_price;
		this->day_close = bar.close_price;
		this->day_low = bar.low_price;
	}
	// Today
	else
	{
		this->day_high = std::max(this->day_high, bar.high_price);
		this->day_low = std::min(this->day_low, bar.low_price);
		this->day_close = bar.close_price;
	}

	if (!this->sell_setup)
		return;

	auto result = am->donchian(this->donchian_window);
	this->tend_high = std::get<0>(result);
	this->tend_low = std::get<1>(result);

	if (this->pos == 0)
	{
		this->intra_trade_low = bar.low_price;
		this->intra_trade_high = bar.high_price;

		if (this->tend_high > this->sell_setup)
		{
			float long_entry = std::max(this->buy_break, this->day_high);
			this->Buy(long_entry, this->fixed_size, true);

			this->Short(this->sell_enter, this->multiplier * this->fixed_size, true);
		}
		else if (this->tend_low < this->buy_setup)
		{
			float short_entry = std::min(this->sell_break, this->day_low);
			this->Short(short_entry, this->fixed_size, true);

			this->Buy(this->buy_enter, this->multiplier * this->fixed_size, true);
		}
	}

	else if (this->pos > 0)
	{
		this->intra_trade_high = std::max(this->intra_trade_high, bar.high_price);
		float long_stop = this->intra_trade_high * (1 - this->trailing_long / 100);
		this->Sell(long_stop, abs(this->pos), true);
	}
	else if (this->pos < 0)
	{
		this->intra_trade_low = std::min(this->intra_trade_low, bar.low_price);
		float short_stop = this->intra_trade_low * (1 + this->trailing_short / 100);
		this->Cover(short_stop, abs(this->pos), true);
	}
	
	this->put_event();
}

void RBreakStrategy::on_stop()
{
	write_log("Strategy Stop");

	// Close existing position
// 	if (this->pos > 0)
// 		this->Sell(bar.close_price * 0.99, abs(this->pos));
// 	else if (this->pos < 0)
// 		this->Cover(bar.close_price * 1.01, abs(this->pos));
}

void RBreakStrategy::on_tick(const TickData& tick)
{
	bg->update_tick(tick);
}

void RBreakStrategy::on_trade(const TradeData& trade)
{
	write_log(Printf("on_trade %f %s", trade.volume, trade.symbol.c_str()));
}




const char* GetStrategyClass()
{
	return CLASS_NAME;
}

CtaTemplate* GetStrategyInstance(CtaEngine* cta_engine, const char* strategy_name, const char* kt_symbol, Json setting)
{
	return new RBreakStrategy(cta_engine, strategy_name, kt_symbol, setting);
}

