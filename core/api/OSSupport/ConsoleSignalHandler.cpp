#include "Globals.h"
#include "ConsoleSignalHandler.h"
#include "EventLoop.h"
#include <csignal>


static void NonCtrlHandler(int a_Signal)
{
	LOGD("Terminate event raised from std::signal");

	switch (a_Signal)
	{
	case SIGSEGV:
	{
		PrintStackTrace();

		LOGERROR(
			"Failure report: \n\n"
			"  :(   | Cuberite has encountered an error and needs to close\n"
			"       | SIGSEGV: Segmentation fault\n"
			"       |\n"
#ifdef BUILD_ID
			"       | Cuberite " BUILD_SERIES_NAME " (id: " BUILD_ID ")\n"
			"       | from commit " BUILD_COMMIT_ID "\n"
#endif
		);

		std::signal(SIGSEGV, SIG_DFL);
		return;
	}
	case SIGABRT:
#ifdef SIGABRT_COMPAT
	case SIGABRT_COMPAT:
#endif
	{
		PrintStackTrace();

		LOGERROR(
			"Failure report: \n\n"
			"  :(   | Cuberite has encountered an error and needs to close\n"
			"       | SIGABRT: Server self-terminated due to an internal fault\n"
			"       |\n"
#ifdef BUILD_ID
			"       | Cuberite " BUILD_SERIES_NAME " (id: " BUILD_ID ")\n"
			"       | from commit " BUILD_COMMIT_ID "\n"
#endif
		);

		std::signal(SIGSEGV, SIG_DFL);
		return;
	}
	case SIGINT:
	case SIGTERM:
	{
		Keen::api::exit_main_event_loop();
		return;
	}
	}
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif





#ifdef _WIN32

/** Handle CTRL events in windows, including console window close. */
static BOOL CtrlHandler(DWORD fdwCtrlType)
{
	Keen::api::exit_main_event_loop();
	LOGD("Terminate event raised from the Windows CtrlHandler");

	return TRUE;
}

#endif





namespace ConsoleSignalHandler
{
	void Register()
	{
		std::signal(SIGSEGV, NonCtrlHandler);
		std::signal(SIGTERM, NonCtrlHandler);
		std::signal(SIGINT, NonCtrlHandler);
		std::signal(SIGABRT, NonCtrlHandler);
#ifdef SIGABRT_COMPAT
		std::signal(SIGABRT_COMPAT, NonCtrlHandler);
#endif
#ifdef SIGPIPE
		std::signal(SIGPIPE, SIG_IGN);  // Ignore (PR #2487).
#endif

#ifdef _WIN32
		SetConsoleCtrlHandler(reinterpret_cast<PHANDLER_ROUTINE>(CtrlHandler), TRUE);
#endif
	}
};