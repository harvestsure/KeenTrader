#pragma once

namespace Keen
{
	namespace engine
	{
		class EventEmitter;
		class TradeEngine;
		class BaseEngine;

		class KEEN_TRADER_EXPORT BaseApp
		{
		public:
			BaseApp();
			virtual ~BaseApp();

		public:
			virtual const AString appName() const = 0;
			virtual BaseEngine* engine_class(TradeEngine* trade_engine, EventEmitter* event_emitter) = 0;
		};
	}
}
