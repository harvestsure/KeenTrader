#include <api/Globals.h>
#include "template.h"
#include "engine.h"
#include "base.h"

using namespace std::placeholders;

namespace Keen
{
	namespace app
	{
		CtaTemplate::CtaTemplate(
			CtaEngine* cta_engine,
			AString strategy_name,
			AString kt_symbol,
			Json setting)
		{
			this->cta_engine = cta_engine;
			this->strategy_name = strategy_name;
			this->kt_symbol = kt_symbol;

			this->inited = false;
			this->trading = false;
			this->pos = 0;

			// Copy a new variables list here to avoid duplicate insert when multiple
			// strategy instances are created with the same strategy class.
			//this->variables = copy(this->variables);
			this->variables.push_back("inited");
			this->variables.push_back("trading");
			this->variables.push_back("pos");

			this->update_setting(setting);
		}

		CtaTemplate::~CtaTemplate()
		{

		}

		void CtaTemplate::update_setting(Json setting)
		{
			/*
			* Update strategy parameter wtih value in setting dict.
			*/
// 				for (AString name : this->parameters)
// 				{
// 					if (setting.count(name))
// 						setattr(name, setting[name]);
// 				}
		}

		void CtaTemplate::get_class_parameters(/*cls*/)
		{
			/*
			* Get default parameters dict of strategy class.
			*/
// 				class_parameters = {};
// 				for name in cls.parameters :
// 				class_parameters[name] = getattr(cls, name);
// 				return class_parameters;
		}

		void CtaTemplate::get_parameters()
		{
			/*
			* Get strategy parameters dict.
			*/
// 				strategy_parameters = {};
// 				for (AString name : this->parameters)
// 				strategy_parameters[name] = getattr(name);
// 				return strategy_parameters;
		}

		Json CtaTemplate::get_variables()
		{
			/*
			* Get strategy variables dict.
			*/
			Json strategy_variables = {};
			// 				for name in this->variables :
			// 				strategy_variables[name] = getattr(name);
			return strategy_variables;
		}

		Json CtaTemplate::get_data()
		{
			/*
			* Get strategy data.
			*/
			Json strategy_data = {
				{ "strategy_name", this->strategy_name },
				{ "kt_symbol" , this->kt_symbol },
				//{ "class_name" , this->__class__.__name__ },
				{ "author" , this->author },
				//{ "parameters" , this->get_parameters() },
				//{ "variables" , this->get_variables() }
			};
			return strategy_data;
		}

		void CtaTemplate::on_init()
		{
			/*
			* Callback when strategy is inited.
			*/
		}

		void CtaTemplate::on_start()
		{
			/*
			* Callback when strategy is started.
			*/
		}

		void CtaTemplate::on_stop()
		{
			/*
			* Callback when strategy is stopped.
			*/
		}

		void CtaTemplate::on_tick(const TickData& tick)
		{
			/*
			* Callback of new tick data update.
			*/
		}

		void CtaTemplate::on_bar(const BarData& bar)
		{
			/*
			* Callback of new bar data update.
			*/
		}

		void CtaTemplate::on_trade(const TradeData& trade)
		{
			/*
			* Callback of new trade data update.
			*/
		}

		void CtaTemplate::on_order(const OrderData& order)
		{
			/*
			* Callback of new order data update.
			*/
		}

		void CtaTemplate::on_stop_order(const StopOrder& stop_order)
		{
			/*
			* Callback of stop order update.
			*/
		}

		AStringList CtaTemplate::Buy(float price, float volume, bool stop/* = false*/, bool lock/* = false*/, bool net/* = false*/)
		{
			/*
			* Send buy order to open a long position.
			*/
			return this->send_order(Direction::LONG, Offset::OPEN, price, volume, stop, lock, net);
		}

		AStringList CtaTemplate::Sell(float price, float volume, bool stop/* = false*/, bool lock/* = false*/, bool net/* = false*/)
		{
			/*
			* Send sell order to close a long position.
			*/
			return this->send_order(Direction::SHORT, Offset::CLOSE, price, volume, stop, lock, net);
		}

		AStringList CtaTemplate::Short(float price, float volume, bool stop/* = false*/, bool lock/* = false*/, bool net/* = false*/)
		{
			/*
			* Send short order to open as short position.
			*/
			return this->send_order(Direction::SHORT, Offset::OPEN, price, volume, stop, lock, net);
		}

		AStringList CtaTemplate::Cover(float price, float volume, bool stop/* = false*/, bool lock/* = false*/, bool net/* = false*/)
		{
			/*
			* Send cover order to close a short position.
			*/
			return this->send_order(Direction::LONG, Offset::CLOSE, price, volume, stop, lock, net);
		}

		AStringList CtaTemplate::send_order(
			Direction direction,
			Offset offset,
			float price,
			float volume,
			bool stop/* = false*/,
			bool lock/* = false*/,
			bool net/* = false*/
		)
		{
			/*
			* Send a new order.
			*/
			if (this->trading)
			{
				AStringList kt_orderids = this->cta_engine->send_order(this, direction, offset, price, volume, stop, lock, net);
				return kt_orderids;
			}
			else
			{
				return AStringList();
			}
		}

		void CtaTemplate::cancel_order(AString kt_orderid)
		{
			/*
			* Cancel an existing order.
			*/
			if (this->trading)
				this->cta_engine->cancel_order(this, kt_orderid);
		}

		void CtaTemplate::cancel_all()
		{
			/*
			* Cancel all orders sent by strategy.
			*/
			if (this->trading)
				this->cta_engine->cancel_all(this);
		}

		void CtaTemplate::write_log(AString msg)
		{
			/*
			* Write a log message.
			*/
			this->cta_engine->write_log(msg, this);
		}

		EngineType CtaTemplate::get_engine_type()
		{
			/*
			* Return whether the cta_engine is backtesting or live trading.
			*/
			return this->cta_engine->get_engine_type();
		}

		float CtaTemplate::get_pricetick()
		{
			return this->cta_engine->get_pricetick(this);
		}

		int CtaTemplate::get_size()
		{
			return this->cta_engine->get_size(this);
		}

		void CtaTemplate::load_bar(int days, 
			Interval interval/* = Interval::MINUTE*/, 
			FnMut<void(BarData)> callback/* = nullptr*/, 
			bool use_database/* = false*/)
		{
			/*
			* Load historical bar data for initializing strategy.
			*/
			if (!callback)
				callback = std::bind(&CtaTemplate::on_bar, this, _1);

			this->cta_engine->load_bar(this->kt_symbol, days, interval, callback, use_database);
		}

		void CtaTemplate::put_event()
		{
			/*
			* Put an strategy data event for ui update.
			*/
			if (this->inited)
				this->cta_engine->put_strategy_event(this);
		}

		void CtaTemplate::send_email(AString msg)
		{
			/*
			* Send email to default receiver.
			*/
			if (this->inited)
				this->cta_engine->send_email(msg, this);
		}

		void CtaTemplate::sync_data()
		{
			/*
			* Sync strategy variables value into disk storage.
			*/
			if (this->trading)
				this->cta_engine->sync_strategy_data(this);
		}




		CtaSignal::CtaSignal()
		{
			this->signal_pos = 0;
		}

		void CtaSignal::on_tick(TickData tick)
		{
			/*
			* Callback of new tick data update.
			*/
		}

		void CtaSignal::on_bar(BarData bar)
		{
			/*
			* Callback of new bar data update.
			*/
		}

		void CtaSignal::set_signal_pos(int pos)
		{
			this->signal_pos = pos;
		}

		int CtaSignal::get_signal_pos()
		{
			return this->signal_pos;
		}





		TargetPosTemplate::TargetPosTemplate(CtaEngine* cta_engine,
			AString strategy_name,
			AString kt_symbol,
			Json setting)
			: CtaTemplate(cta_engine, strategy_name, kt_symbol, setting)
		{
			this->variables.push_back("target_pos");
		}

		void TargetPosTemplate::on_tick(const TickData& tick)
		{
			/*
			* Callback of new tick data update.
			*/
			this->last_tick = tick;
		}

		void TargetPosTemplate::on_bar(const BarData& bar)
		{
			/*
			* Callback of new bar data update.
			*/
			this->last_bar = bar;
		}

		void TargetPosTemplate::on_order(const OrderData& order)
		{
			/*
			* Callback of new order data update.
			*/
			AString kt_orderid = order.kt_orderid;

			if (!order.is_active())
			{
				if (active_orderids.count(kt_orderid))
				{
					active_orderids.erase(kt_orderid);
				}

				if (cancel_orderids.count(kt_orderid))
				{
					cancel_orderids.erase(kt_orderid);
				}
			}
		}

		bool TargetPosTemplate::check_order_finished()
		{
			return active_orderids.size();
		}

		void TargetPosTemplate::set_target_pos(int target_pos)
		{
			this->target_pos = target_pos;
			this->trade();
		}

		void TargetPosTemplate::cancel_old_order()
		{
			for (auto kt_orderid : active_orderids)
			{
				if (!cancel_orderids.count(kt_orderid))
				{
					this->cancel_order(kt_orderid);
					cancel_orderids.insert(kt_orderid);
				}
			}
		}

		void TargetPosTemplate::send_new_order()
		{
			int pos_change = this->target_pos - this->pos;
			if (!pos_change)
				return;

			float long_price = 0;
			float short_price = 0;

			AStringList kt_orderids;

			if (this->last_tick)
			{
				if (pos_change > 0)
				{
					long_price = this->last_tick->ask_price_1 + this->tick_add;
					if (this->last_tick->limit_up)
						long_price = std::min(long_price, this->last_tick->limit_up);
				}
				else
				{
					short_price = this->last_tick->bid_price_1 - this->tick_add;
					if (this->last_tick->limit_down)
						short_price = std::max(short_price, this->last_tick->limit_down);
				}
			}
			else
			{
				if (pos_change > 0)
					long_price = this->last_bar->close_price + this->tick_add;
				else
					short_price = this->last_bar->close_price - this->tick_add;
			}

			if (this->get_engine_type() == EngineType::BACKTESTING)
			{
				if (pos_change > 0)
					kt_orderids = this->Buy(long_price, abs(pos_change));
				else
					kt_orderids = this->Short(short_price, abs(pos_change));
				this->active_orderids.insert(kt_orderids.begin(), kt_orderids.end());
			}
			else
			{
				if (this->active_orderids.size())
					return;

				if (pos_change > 0)
				{
					if (this->pos < 0)
					{
						if (pos_change < abs(this->pos))
							kt_orderids = this->Cover(long_price, pos_change);
						else
							kt_orderids = this->Cover(long_price, abs(this->pos));
					}
					else
						kt_orderids = this->Buy(long_price, abs(pos_change));
				}
				else
				{
					if (this->pos > 0)
					{
						if (abs(pos_change) < this->pos)
							kt_orderids = this->Sell(short_price, abs(pos_change));
						else
							kt_orderids = this->Sell(short_price, abs(this->pos));
					}
					else
					{
						kt_orderids = this->Short(short_price, abs(pos_change));
					}
				}
				this->active_orderids.insert(kt_orderids.begin(), kt_orderids.end());
			}
		}

		void TargetPosTemplate::trade()
		{
			if (!this->check_order_finished())
				this->cancel_old_order();
			else
				this->send_new_order();
		}
	}
}

