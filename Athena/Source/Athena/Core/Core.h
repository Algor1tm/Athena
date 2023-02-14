#pragma once

#include <memory>
#include <string>
#include <filesystem>


// Platform detection using predefined macros
#ifdef _WIN32
	/* Windows x64/x86 */
	#ifdef _WIN64
		/* Windows x64  */
		#define ATN_PLATFORM_WINDOWS
	#else
	/* Windows x86 */
		#error "x86 Builds are not supported!"
	#endif
#elif defined(__APPLE__) || defined(__MACH__)
	#include <TargetConditionals.h>
	/* TARGET_OS_MAC exists on all the platforms
	 * so we must check all of them (in this order)
	 * to ensure that we're running on MAC
	 * and not some other Apple platform */
	#if TARGET_IPHONE_SIMULATOR == 1
		#error "IOS simulator is not supported!"
	#elif TARGET_OS_IPHONE == 1
		#define ATN_PLATFORM_IOS
		#error "IOS is not supported!"
	#elif TARGET_OS_MAC == 1
		#define ATN_PLATFORM_MACOS
		#error "MacOS is not supported!"
	#else
		#error "Unknown Apple platform!"
	#endif
 /* We also have to check __ANDROID__ before __linux__
  * since android is based on the Linux kernel
  * it has __linux__ defined */
#elif defined(__ANDROID__)
	#define ATN_PLATFORM_ANDROID
	#error "Android is not supported!"
#elif defined(__linux__)
	#define ATN_PLATFORM_LINUX
	#error "Linux is not supported!"
#else
	/* Unknown compiler/platform */
	#error "Unknown platform!"
#endif // End of platform detection


// Type of linking detection
#ifdef ATN_PLATFORM_WINDOWS
	#ifdef ATN_BUILD_DLL
		#define ATHENA_API __declspec(dllexport)
	#else
		#define  ATHENA_API __declspec(dllimport)
	#endif
#else
	#error Athena only supports Windows!
#endif // End of linking detection


#ifdef ATN_DEBUG
	#if defined(ATN_PLATFORM_WINDOWS)
		#define ATN_DEBUGBREAK() __debugbreak()
	#elif defined(ATN_PLATFORM_LINUX)
		#include <signal.h>
		#define ATN_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif

	#define ATN_ASSERTS

#else
	#define ATN_DEBUGBREAK()
#endif


#ifdef _MSC_VER
	#define ATN_FORCEINLINE __forceinline
#elif __GNUC__
	#define ATN_FORCEINLINE __attribute__((always_inline))
#else 
	#define ATN_FORCEINLINE inline
#endif

#define ATN_EXPAND_MACRO(x) x
#define ATN_STRINGIFY_MACRO(x) #x


#ifdef ATN_ASSERTS
	// Alternatively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
	// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define ATN_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { ATN##type##ERROR(msg, __VA_ARGS__); ATN_DEBUGBREAK(); } }
	#define ATN_INTERNAL_ASSERT_WITH_MSG(type, check, ...) ATN_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define ATN_INTERNAL_ASSERT_NO_MSG(type, check) ATN_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", ATN_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define ATN_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define ATN_INTERNAL_ASSERT_GET_MACRO(...) ATN_EXPAND_MACRO( ATN_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, ATN_INTERNAL_ASSERT_WITH_MSG, ATN_INTERNAL_ASSERT_NO_MSG) )

	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define ATN_ASSERT(...) ATN_EXPAND_MACRO( ATN_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define ATN_CORE_ASSERT(...) ATN_EXPAND_MACRO( ATN_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define ATN_ASSERT(x, ...) 
	#define ATN_CORE_ASSERT(x, ...)
#endif


#define BIT(x) (1 << x)

#define ATN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }


namespace Athena
{
	using byte = ::std::byte; // type of size 1 byte

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

	template <typename T>
	using Scope = ::std::unique_ptr<T>;
	template<typename T, typename... Args>
	constexpr Scope<T> CreateScope(Args&&... args)
	{
		return ::std::make_unique<T>(std::forward<Args>(args)...);
	}

	template <typename T>
	using Ref = ::std::shared_ptr<T>;
	template<typename T, typename... Args>
	constexpr Ref<T> CreateRef(Args&&... args)
	{
		return ::std::make_shared<T>(std::forward<Args>(args)...);
	}
}
