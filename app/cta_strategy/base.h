#pragma once
#include <engine/object.h>

namespace Keen
{
	using namespace engine;

	namespace app
	{
		/*
		* Defines constants and objects used in CtaStrategy App.
		*/
		const AString STOPORDER_PREFIX = "STOP";


		enum class StopOrderStatus
		{
			WAITING,
			CANCELLED,
			TRIGGERED,
		};


		enum class EngineType
		{
			LIVE,
			BACKTESTING,
		};

		enum class BacktestingMode
		{
			BAR = 1,
			TICK = 2,
		};


		class StopOrder
		{
		public:
			AString kt_symbol;
			Direction direction;
			Offset offset;
			float price;
			float volume;
			AString stop_orderid;
			AString strategy_name;
			DateTime datetime;
			bool lock = false;
			bool net = false;
			AStringList kt_orderids;
			StopOrderStatus status = StopOrderStatus::WAITING;
		};

		extern KEEN_APP_EXPORT const AString EVENT_CTA_LOG;
		extern KEEN_APP_EXPORT const AString EVENT_CTA_STRATEGY;
		extern KEEN_APP_EXPORT const AString EVENT_CTA_STOPORDER;

		extern std::map<Interval, std::chrono::minutes> INTERVAL_DELTA_MAP;
	}
}
