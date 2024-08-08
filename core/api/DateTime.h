#pragma once

using namespace std::chrono;

using DateTime = time_point<system_clock, milliseconds>;

extern KEEN_API_EXPORT DateTime DateTimeFromTimestamp(int64_t timestamp);

extern KEEN_API_EXPORT DateTime DateTimeFromStringTime(AString timestamp);

extern KEEN_API_EXPORT AString DateTimeToString(const DateTime& date_time);

extern KEEN_API_EXPORT AString DateTimeToString(const DateTime& date_time, AString format);

extern KEEN_API_EXPORT DateTime StringToDateTime(const AString& strTime);

extern KEEN_API_EXPORT DateTime currentDateTime();

