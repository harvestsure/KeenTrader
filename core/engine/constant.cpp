#include <api/Globals.h>
#include "constant.h"
#include <magic_enum/magic_enum_all.hpp>

namespace Keen
{
	namespace engine
	{
		AString str_direction(Direction direction)
		{
			auto str_value = magic_enum::enum_name(direction);
			return AString(str_value);
		}

		AString exchange_to_str(Exchange exchange)
		{
			auto str_value = magic_enum::enum_name(exchange);
			return AString(str_value);
		}

		AString status_to_str(Status status)
		{
			auto str_value = magic_enum::enum_name(status);
			return AString(str_value);
		}

		AString interval_to_str(Interval interval)
		{
			auto str_value = magic_enum::enum_name(interval);
			return AString(str_value);
		}

		Exchange str_to_exchange(AString exchange)
		{
			auto enum_value = magic_enum::enum_cast<Exchange>(exchange);
			ASSERT(enum_value.has_value());

			return enum_value.value();
		}

		Interval str_to_interval(AString interval)
		{
			auto enum_value = magic_enum::enum_cast<Interval>(interval);
			ASSERT(enum_value.has_value());

			return enum_value.value();
		}
	}
}
