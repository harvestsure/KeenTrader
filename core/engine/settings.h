#pragma once

namespace Keen
{
	namespace engine
	{
		extern Json SETTINGS;

		extern KEEN_ENGINE_EXPORT Json get_settings(AString prefix = "");
	}
}
