#pragma once

// Compiler-dependent stuff:
#if defined(_MSC_VER)
	// Use non-standard defines in <cmath>
	#define _USE_MATH_DEFINES

	#ifndef NDEBUG
		// Override the "new" operator to include file and line specification for debugging memory leaks
		// Ref.: https://social.msdn.microsoft.com/Forums/en-US/ebc7dd7a-f3c6-49f1-8a60-e381052f21b6/debugging-memory-leaks?forum=vcgeneral#53f0cc89-62fe-45e8-bbf0-56b89f2a1901
		// This causes MSVC Debug runs to produce a report upon program exit, that contains memory-leaks
		// together with the file:line information about where the memory was allocated.
		// Note that this doesn't work with placement-new, which needs to temporarily #undef the macro
		// (See AllocationPool.h for an example).
		#define _CRTDBG_MAP_ALLOC
		#include <stdlib.h>
		#include <crtdbg.h>
		#define DEBUG_CLIENTBLOCK   new(_CLIENT_BLOCK, __FILE__, __LINE__)
		// #define new DEBUG_CLIENTBLOCK
		// For some reason this works magically - each "new X" gets replaced as "new(_CLIENT_BLOCK, "file", line) X"
		// The CRT has a definition for this operator new that stores the debugging info for leak-finding later.
	#endif

	#define UNREACHABLE_INTRINSIC __assume(false)

#elif defined(__GNUC__)
	// TODO: Can GCC explicitly mark classes as abstract (no instances can be created)?
	#define abstract

	#define UNREACHABLE_INTRINSIC __builtin_unreachable()
#else
	#error "You are using an unsupported compiler, you might need to #define some stuff here for your compiler"

#endif

// OS-dependent stuff:
#ifdef _WIN32
	#define NOMINMAX  // Windows SDK defines min and max macros, messing up with our std::min and std::max usage.
	#define WIN32_LEAN_AND_MEAN

	// Use CryptoAPI primitives when targeting a version that supports encrypting with AES-CFB8 smaller than a full block at a time.
	#define PLATFORM_CRYPTOGRAPHY (_WIN32_WINNT >= 0x0602)

	#include <Windows.h>
	#include <winsock2.h>
	#include <Ws2tcpip.h>  // IPv6 stuff

	// Windows SDK defines GetFreeSpace as a constant, probably a Win16 API remnant:
	#ifdef GetFreeSpace
		#undef GetFreeSpace
	#endif  // GetFreeSpace
#else
	#define PLATFORM_CRYPTOGRAPHY 0

	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
	#include <unistd.h>
#endif

// CRT stuff:
#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstdarg>
#include <cstddef>

// STL stuff:
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <variant>
#include <future>
#include <functional>


// Custom headers
#include "Defines.h"
#include "StringUtils.h"
#include "DateTime.h"
#include "EventLoop.h"


