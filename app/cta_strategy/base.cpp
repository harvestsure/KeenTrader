#include <api/Globals.h>
#include "base.h"

namespace Keen
{
	namespace app
	{
		const char* EVENT_CTA_LOG = "eCtaLog";
		const char* EVENT_CTA_STRATEGY = "eCtaStrategy";
		const char* EVENT_CTA_STOPORDER = "eCtaStopOrder";

		std::map<Interval, std::chrono::minutes> INTERVAL_DELTA_MAP = {
				{ Interval::MINUTE, std::chrono::minutes{ 1 } },
				{ Interval::HOUR, std::chrono::hours{ 1 } },
				{ Interval::DAILY, std::chrono::hours{ 24 } }
		};
	}
}


