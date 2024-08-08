#include <api/Globals.h>
#include "cta_strategy.h"
#include "base.h"
#include "engine.h"

namespace Keen
{
	namespace app
	{
		CtaStrategyApp::CtaStrategyApp()
		{
		}

		CtaStrategyApp::~CtaStrategyApp()
		{
		}

		const AString CtaStrategyApp::appName() const
		{
			return APP_NAME;
		}

		BaseEngine* CtaStrategyApp::engine_class(TradeEngine* trade_engine, EventEmitter* event_emitter)
		{
			return new CtaEngine(trade_engine, event_emitter);
		}
	}
}
