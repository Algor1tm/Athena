#pragma once

// Type of linking detection
#if ATN_PLATFORM_WINDOWS
	#if ATN_BUILD_DLL
		#define ATHENA_API __declspec(dllexport)
	#else
		#define  ATHENA_API __declspec(dllimport)
	#endif
#else
	#define ATHENA_API
#endif // End of linking detection


// Configuration
#if ATN_DEBUG
	#define ATN_ENABLE_DEBUGBREAK 1
	#define ATN_ENABLE_CHECKS 1
	#define ATN_ENABLE_ENSURES 1
	#define ATN_ENABLE_PROFILING 1
	#define ATN_LOG_LEVEL_DEBUG 1
#elif ATN_RELEASE
	#define ATN_ENABLE_DEBUGBREAK 1
	#define ATN_ENABLE_CHECKS 0
	#define ATN_ENABLE_ENSURES 1
	#define ATN_ENABLE_PROFILING 1
	#define ATN_LOG_LEVEL_DEBUG 0 
#elif ATN_DIST
	#define ATN_ENABLE_DEBUGBREAK 0
	#define ATN_ENABLE_CHECKS 0
	#define ATN_ENABLE_ENSURES 0
	#define ATN_ENABLE_PROFILING 0
	#define ATN_LOG_LEVEL_DEBUG 0 
#endif


#include <string>
#include <filesystem>


#define BIT(x) (1 << x)
#define ATN_EXPAND_MACRO(x) x
#define ATN_STRINGIFY_MACRO(x) #x

#ifndef ATN_PLATFORM_WINDOWS
	#define TEXT(quote) L##quote
#endif

#if ATN_ENABLE_DEBUGBREAK
	#if ATN_PLATFORM_WINDOWS
		#define ATN_DEBUGBREAK() __debugbreak()
	#elif ATN_PLATFORM_LINUX
		#include <signal.h>
		#define ATN_DEBUGBREAK() raise(SIGTRAP)
	#else
		#define ATN_DEBUGBREAK()
	#endif
#else
	#define ATN_DEBUGBREAK()
#endif


#if ATN_ENABLE_CHECKS || ATN_ENABLE_ENSURES
	#define ATN_INTERNAL_ASSERT_IMPL(cond, msg, ...) { if(!(cond)) { ATN_CORE_ERROR(msg __VA_OPT__(,) __VA_ARGS__); ATN_DEBUGBREAK(); } }
	#define ATN_INTERNAL_FATAL_ASSERT_IMPL(cond, name) ATN_INTERNAL_ASSERT_IMPL(cond, name " '{0}' failed at {1}:{2}", ATN_STRINGIFY_MACRO(cond), std::filesystem::path(__FILE__).filename().string(), __LINE__)
#endif

#if ATN_ENABLE_CHECKS
	#define check(cond, msg, ...) ATN_INTERNAL_ASSERT_IMPL(cond, msg, __VA_ARGS__)
	#define checkf(cond) ATN_INTERNAL_FATAL_ASSERT_IMPL(cond, "Assertion")
#else
	#define check(cond, msg, ...)
	#define checkf(cond)
#endif

#if ATN_ENABLE_ENSURES
	#define ensure(cond, msg, ...) ATN_INTERNAL_ASSERT_IMPL(cond, msg, __VA_ARGS__)
	#define ensuref(cond) ATN_INTERNAL_FATAL_ASSERT_IMPL(cond, "Ensure")
#else
	#define ensure(cond, msg, ...)
	#define ensuref(cond)
#endif

#define ATN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }


namespace Athena
{
	using byte = ::std::uint8_t; // type of size 1 byte

	using int8  = ::std::int8_t;  // 8-bit int
	using int16 = ::std::int16_t; // 16-bit int
	using int32 = ::std::int32_t; // 32-bit int
	using int64 = ::std::int64_t; // 64-bit int

	using uint8  = ::std::uint8_t;  // 8-bit unsigned int
	using uint16 = ::std::uint16_t; // 16-bit unsigned int
	using uint32 = ::std::uint32_t; // 32-bit unsigned int
	using uint64 = ::std::uint64_t; // 64-bit unsigned int

	using String = ::std::string; // string type
	using FilePath = ::std::filesystem::path; // filepath type
}

#include "Memory.h"
#include "Profile.h"
