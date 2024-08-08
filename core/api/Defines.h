#pragma once

#ifdef _MSC_VER
    #define KEEN_DECL_EXPORT __declspec(dllexport)
    #define KEEN_DECL_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    #define KEEN_DECL_EXPORT __attribute__((visibility("default")))
    #define KEEN_DECL_IMPORT __attribute__((visibility("default")))
#else
    #define KEEN_DECL_EXPORT
    #define KEEN_DECL_IMPORT
#endif

#if defined(KEEN_SHARED) || !defined(KEEN_STATIC)
#  ifdef KEEN_STATIC
#    error "Both KEEN_SHARED and KEEN_STATIC defined, please make up your mind"
#  endif
#  ifndef KEEN_SHARED
#    define KEEN_SHARED
#  endif
#  if defined(KEEN_BUILD_API_LIB)
#    define KEEN_API_EXPORT KEEN_DECL_EXPORT
#  else
#    define KEEN_API_EXPORT KEEN_DECL_IMPORT
#  endif
#  if defined(KEEN_BUILD_TRADER_LIB)
#    define KEEN_TRADER_EXPORT KEEN_DECL_EXPORT
#  else
#    define KEEN_TRADER_EXPORT KEEN_DECL_IMPORT
#  endif
#  if defined(KEEN_BUILD_EXCHANGE_LIB)
#    define KEEN_EXCHANGE_EXPORT KEEN_DECL_EXPORT
#  else
#    define KEEN_EXCHANGE_EXPORT KEEN_DECL_IMPORT
#  endif
#  if defined(KEEN_BUILD_APP_LIB)
#    define KEEN_APP_EXPORT KEEN_DECL_EXPORT
#  else
#    define KEEN_APP_EXPORT KEEN_DECL_IMPORT
#  endif
#else
#  define KEEN_API_EXPORT
#  define KEEN_TRADER_EXPORT
#  define KEEN_EXCHANGE_EXPORT
#  define KEEN_APP_EXPORT
#endif


// Integral types with predefined sizes:
typedef signed long long Int64;
typedef signed int       Int32;
typedef signed short     Int16;
typedef signed char      Int8;

typedef unsigned long long UInt64;
typedef unsigned int       UInt32;
typedef unsigned short     UInt16;
typedef unsigned char      UInt8;

typedef unsigned char Byte;

typedef std::string AString;
typedef std::vector<AString> AStringVector;
typedef std::list<AString>   AStringList;
typedef std::set<AString>   AStringSet;

/** A string dictionary, used for key-value pairs. */
typedef std::map<AString, AString> AStringMap;

// Common definitions:

/** Evaluates to the number of elements in an array (compile-time!) */
#define ARRAYCOUNT(X) (sizeof(X) / sizeof(*(X)))

/** Allows arithmetic expressions like "32 KiB" (but consider using parenthesis around it, "(32 KiB)") */
#define KiB * 1024
#define MiB * 1024 * 1024


template <typename Signature>
using FnMut = std::function<Signature>;

using ContiguousByteBuffer = std::basic_string<std::byte>;
using ContiguousByteBufferView = std::basic_string_view<std::byte>;


#include <nlohmann/json.hpp>
// for convenience
using Json = nlohmann::json;

#define SAFE_RELEASE(x) {if (x) {delete x;x = NULL;}}

// A macro to disallow the copy constructor and operator = functions
// This should be used in the declarations for any class that shouldn't allow copying itself
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
	TypeName(const TypeName &) = delete; \
	TypeName & operator =(const TypeName &) = delete


#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wsign-conversion"
	#pragma clang diagnostic ignored "-Wsigned-enum-bitfield"
#endif

#include <fmt/format.h>
#include <fmt/printf.h>

#ifdef __clang__
	#pragma clang diagnostic pop
#endif


#include "LoggerSimple.h"
#include "OSSupport/CriticalSection.h"
#include "OSSupport/Event.h"
#include "OSSupport/File.h"
#include "OSSupport/StackTrace.h"
#include "OSSupport/ConsoleSignalHandler.h"


#ifdef NDEBUG
	#define ASSERT(x)
#else
	#define ASSERT(x) ( !!(x) || ( LOGERROR("Assertion failed: %s, file %s, line %i", #x, __FILE__, __LINE__), std::abort(), 0))
#endif

// Pretty much the same as ASSERT() but stays in Release builds
#define VERIFY(x) (!!(x) || ( LOGERROR("Verification failed: %s, file %s, line %i", #x, __FILE__, __LINE__), std::abort(), 0))


