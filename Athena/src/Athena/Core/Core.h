#pragma once

#include <memory>


#ifdef ATN_PLATFORM_WINDOWS
	#ifdef ATN_DYNAMIC_LINK
		#ifdef ATN_BUILD_DLL
			#define ATHENA_API __declspec(dllexport)
		#else
			#define  ATHENA_API __declspec(dllimport)
		#endif
	#else
		#define ATHENA_API
	#endif
#else
	#error Athena only supports Windows!
#endif

#ifdef ATN_DEBUG
#define ATN_ASSERTS
#endif

#ifdef ATN_ASSERTS
	#define ATN_ASSERT(x, ...)	{ if(!(x)) { ATN_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define ATN_CORE_ASSERT(x, ...)  { if(!(x)) { ATN_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define ATN_ASSERT(x, ...) 
	#define ATN_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define ATN_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)


namespace Athena
{
	template <typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename... Args>
	constexpr Scope<T> CreateScope(Args&&... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template <typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename... Args>
	constexpr Ref<T> CreateRef(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
}
