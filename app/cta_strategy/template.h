#pragma once

#include "base.h"
#include <engine/object.h>
#include <engine/utility.h>


namespace Keen
{
	namespace app
	{
		class CtaEngine;
		class CtaTemplate;

		using TradeData = engine::TradeData;
		using TickData = engine::TickData;
		using OrderData = engine::OrderData;
		using BarData = engine::BarData;
		using Interval = engine::Interval;
		using Exchange = engine::Exchange;
		using Direction = engine::Direction;
		using Offset = engine::Offset;


#define StrategyClass "GetStrategyClass"
#define StrategyInstance "GetStrategyInstance"

		typedef const char*(*fGetStrategyClass)();
		typedef CtaTemplate*(*fGetStrategyInstance)(CtaEngine*, const char*, const char*, Json);


		class KEEN_APP_EXPORT CtaTemplate
		{
		public:
			AString author = "";
			AStringList parameters;
			AStringList variables;

			CtaEngine* cta_engine;
			AString strategy_name;
			AString kt_symbol;

			bool inited;
			bool trading;
			float pos;

			CtaTemplate(
				CtaEngine* cta_engine,
				AString strategy_name,
				AString kt_symbol,
				Json setting);

			virtual ~CtaTemplate();

			virtual AString __class__() = 0;

			void update_setting(Json setting);

			void get_class_parameters();

			void get_parameters();

			Json get_variables();

			Json get_data();

			virtual void on_init();

			virtual void on_start();

			virtual void on_stop();

			virtual void on_tick(const TickData& tick);

			virtual void on_bar(const BarData& bar);

			virtual void on_trade(const TradeData& trade);

			virtual void on_order(const OrderData& order);

			virtual void on_stop_order(const StopOrder& stop_order);

			AStringList Buy(float price, float volume, bool stop = false, bool lock = false, bool net = false);

			AStringList Sell(float price, float volume, bool stop = false, bool lock = false, bool net = false);

			AStringList Short(float price, float volume, bool stop = false, bool lock = false, bool net = false);

			AStringList Cover(float price, float volume, bool stop = false, bool lock = false, bool net = false);

			AStringList send_order(Direction direction, Offset offset, float price, float volume, bool stop = false, bool lock = false, bool net = false);

			void cancel_order(AString kt_orderid);

			void cancel_all();

			void write_log(AString msg);

			EngineType get_engine_type();

			float get_pricetick();

			int get_size();

			void load_bar(int days, Interval interval = Interval::MINUTE, FnMut<void(BarData)> callback = nullptr,bool use_database = false);

			void put_event();

			void send_email(AString msg);

			void sync_data();

			void async_exec();
		};


		class CtaSignal
		{
		public:
			CtaSignal();

			virtual void on_tick(TickData tick);

			virtual void on_bar(BarData bar);

			void set_signal_pos(int pos);

			int get_signal_pos();

		protected:
			int signal_pos;
		};


		class TargetPosTemplate : public CtaTemplate
		{
		public:
			TargetPosTemplate(CtaEngine* cta_engine,
				AString strategy_name,
				AString kt_symbol,
				Json setting);

			void on_tick(const TickData& tick) override;

			void on_bar(const BarData& bar) override;

			void on_order(const OrderData& order) override;

			bool check_order_finished();

			void set_target_pos(int target_pos);

			void cancel_old_order();

			void send_new_order();

			void trade();

		protected:
			int tick_add = 1;

			std::optional<TickData> last_tick = std::nullopt;
			std::optional<BarData> last_bar = std::nullopt;
			int target_pos = 0;

			AStringSet active_orderids;
			AStringSet cancel_orderids;
		};

	}
}
