#pragma once

namespace Keen
{
	namespace engine
	{
		extern Json SETTINGS;

		extern KEEN_TRADER_EXPORT Json get_settings(AString prefix = "");
	}
}
