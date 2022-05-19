#pragma once


#ifdef ATN_PLATFORM_WINDOWS
	#ifdef ATN_BUILD_DLL
		#define ATHENA_API __declspec(dllexport)
	#else
		#define  ATHENA_API __declspec(dllimport)
	#endif
#else
	#error Athena only supports Windows!
#endif

#ifdef ATN_ASSERTS
	#define ATN_ASSERT(x, ...)	{ if(!(x)) { ATN_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define ATN_CORE_ASSERT(x, ...)  { if(!(x)) { ATN_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define ATN_ASSERT(x, ...) 
	#define ATN_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define BIND_MEMBER_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)
