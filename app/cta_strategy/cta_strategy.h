#pragma once

#include <event/event.h>
#include <engine/engine.h>
#include <engine/app.h>

namespace Keen
{
	using namespace engine;

	namespace app
	{
		class KEEN_APP_EXPORT CtaStrategyApp : public BaseApp
		{
		public:
			CtaStrategyApp();
			virtual ~CtaStrategyApp();

			virtual const AString appName() const;
			virtual BaseEngine* engine_class(TradeEngine* trade_engine, EventEmitter* event_emitter);
		};
	}
}
