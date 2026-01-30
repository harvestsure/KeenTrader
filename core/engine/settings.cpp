#include <api/Globals.h>
#include "settings.h"
#include "utility.h"
namespace Keen
{
	namespace engine
	{
		Json SETTINGS = {
			{ "log.active", true },
			//{ "log.level", CRITICAL },
			{ "log.console", true },
			{ "log.file", true },

			{ "email.server", "" },
			{ "email.port", 465 },
			{ "email.username", "" },
			{ "email.password", "" },
			{ "email.sender", "" },
			{ "email.receiver", "" },

			{ "notice.route", "" },
			{ "notice.token", "" },
			{ "notice.chat_id", "" },

			{ "rqdata.username", "" },
			{ "rqdata.password", "" },

			{ "database.driver", "sqlite" },  // see database.Driver
			{ "database.database", "database.db" },  // for sqlite, use this as filepath
			{ "database.host", "localhost" },
			{ "database.port", 3306 },
			{ "database.user", "root" },
			{ "database.password", "" },
			{ "database.authentication_source", "admin" },  // for mongodb
		};

		// Load global setting from json file.
		const AString SETTING_FILENAME = "kt_setting.json";
		
		
		Json get_settings(AString prefix /*= ""*/)
		{
			Json result = Json::object();
			for (auto &[key, value] : SETTINGS.items())
			{
				if (prefix.empty())
				{
					result[key] = value;
				}
				else
				{
					size_t pos = key.find(prefix);
					if (pos != std::string::npos)
					{
						result[key.substr(pos + prefix.length())] = value;
					}
				}
			}

			return result;
		}

		Json loadLocalSettings()
		{
			Json newSetting = load_json(SETTING_FILENAME);
			SETTINGS.merge_patch(newSetting);
			return newSetting;
		}

		Json g_settings = loadLocalSettings();
	}
}

