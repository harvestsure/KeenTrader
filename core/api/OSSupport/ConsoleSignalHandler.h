#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

namespace ConsoleSignalHandler
{
	KEEN_API_EXPORT void Register();
}