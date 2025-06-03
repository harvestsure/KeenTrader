#pragma once

#include "base.h"
#include <engine/engine.h>
#include <engine/engine.h>

class ThreadPool;

namespace Keen
{
	using namespace engine;

	namespace app
	{
		class CtaTemplate;

		const AString APP_NAME = "CtaStrategy";

		class KEEN_APP_EXPORT CtaEngine : public BaseEngine
		{
		public:
			CtaEngine(TradeEngine* trade_engine, EventEmitter* event_emitter);
			virtual ~CtaEngine();

			void init_engine();

			void close();

			void register_event();

			void init_datafeed();

			std::list<BarData> query_bar_from_datafeed(AString symbol, Exchange exchange, Interval interval, DateTime start, DateTime end);

			void process_tick_event(const Event& event);

			void process_order_event(const Event& event);

			void process_trade_event(const Event& event);

			void check_stop_order(const TickData& tick);

			virtual AStringList send_server_order(
				CtaTemplate *strategy,
				ContractData contract,
				Direction direction,
				Offset offset,
				float price,
				float volume,
				OrderType type,
				bool lock,
				bool net
			);

			virtual AStringList send_limit_order(
				CtaTemplate *strategy,
				ContractData contract,
				Direction direction,
				Offset offset,
				float price,
				float volume,
				bool lock,
				bool net
			);

			virtual AStringList send_server_stop_order(
				CtaTemplate *strategy,
				ContractData contract,
				Direction direction,
				Offset offset,
				float price,
				float volume,
				bool lock,
				bool net
			);

			virtual AStringList send_local_stop_order(
				CtaTemplate *strategy,
				Direction direction,
				Offset offset,
				float price,
				float volume,
				bool lock,
				bool net
			);

			virtual void cancel_server_order(CtaTemplate *strategy, AString kt_orderid);

			virtual void cancel_local_stop_order(CtaTemplate *strategy, AString stop_orderid);

			virtual AStringList send_order(
				CtaTemplate *strategy,
				Direction direction,
				Offset offset,
				float price,
				float volume,
				bool stop,
				bool lock,
				bool net
			);

			virtual void cancel_order(CtaTemplate *strategy, AString kt_orderid);

			virtual void cancel_all(CtaTemplate *strategy);

			virtual EngineType get_engine_type();

			virtual float get_pricetick(CtaTemplate* strategy);

			virtual int get_size(CtaTemplate* strategy);

			virtual std::list<BarData> load_bar(AString kt_symbol, int days, Interval interval, FnMut<void(BarData)> callback = nullptr, bool use_database = false);

			void call_strategy_func(CtaTemplate *strategy, FnMut<void(std::any)> func, std::any params = nullptr);

			virtual void add_strategy(AString class_name, AString strategy_name, AString kt_symbol, Json setting);

			void init_strategy(AString strategy_name);

			void _init_strategy(AString strategy_name);

			void start_strategy(AString strategy_name);

			void stop_strategy(AString strategy_name);

			void edit_strategy(AString strategy_name, Json setting);

			bool remove_strategy(AString strategy_name);

			void load_strategy_class();

			void load_strategy_class_from_folder(AString root_path);

			void load_strategy_class_from_module(AString module_name);

			void load_strategy_data();

			void sync_strategy_data(CtaTemplate *strategy);

			AStringList get_all_strategy_class_names();

			void get_strategy_class_parameters(AString class_name);

			void get_strategy_parameters(AString strategy_name);

			void init_all_strategies();

			void start_all_strategies();

			void stop_all_strategies();

			void load_strategy_setting();

			void update_strategy_setting(AString strategy_name, Json setting);

			void remove_strategy_setting(AString strategy_name);

			void put_stop_order_event(StopOrder stop_order);

			void put_strategy_event(CtaTemplate *strategy);

			virtual void write_log(AString msg, CtaTemplate* strategy = nullptr);

			virtual void send_email(AString msg, CtaTemplate* strategy = nullptr);

		protected:
			EngineType engine_type;                        // live trading engine

			static AString setting_filename;
			static AString data_filename;

			Json strategy_setting;                         // strategy_name : dict
			Json strategy_data;                            // strategy_name : dict

			std::map<AString, std::any> classes;                            // class_name : stategy_class
			std::map<AString, CtaTemplate*> strategies;                     // strategy_name : strategy

			std::map<AString, std::list<CtaTemplate*>> symbol_strategy_map; // kt_symbol : strategy list
			std::map<AString, CtaTemplate*> orderid_strategy_map;           // kt_orderid : strategy
			std::map<AString, AStringSet> strategy_orderid_map;             // strategy_name : orderid list

			int stop_order_count = 0;                                       // for generating stop_orderid
			std::map<AString, StopOrder> stop_orders;                       // stop_orderid: stop_order\

			AStringSet kt_tradeids;                                         // for filtering duplicate trade
		};
	}
}
