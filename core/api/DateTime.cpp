#include "Globals.h"
#include "DateTime.h"
#include <date/date.h>


DateTime DateTimeFromTimestamp(int64_t timestamp)
{
	auto mTime = ::milliseconds(timestamp);
	auto tp = time_point<system_clock, milliseconds>(mTime);
	return tp;
}

DateTime DateTimeFromStringTime(AString timestamp)
{
	auto mTime = ::milliseconds(std::atoll(timestamp.c_str()));
	auto tp = time_point<system_clock, milliseconds>(mTime);
	return tp;
}

AString DateTimeToString(const DateTime& date_time)
{
	AString ret = date::format("%FT%TZ", date_time);

	return ret;
}

AString DateTimeToString(const DateTime& date_time, AString format)
{
	AString ret = date::format(format, date_time);

	return ret;
}

DateTime StringToDateTime(const AString& strTime)
{
	std::istringstream in{ strTime };
	date::sys_time<milliseconds> tp;

	in >> date::parse("%FT%TZ", tp);
	ASSERT(!in.fail());
	ASSERT(!in.bad());

	return tp;
}

DateTime currentDateTime()
{
	return time_point_cast<milliseconds>(system_clock::now());
}

