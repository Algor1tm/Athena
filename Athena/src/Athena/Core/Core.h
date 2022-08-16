#pragma once

#include <memory>
#include <string>

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
  * since android is based on the linux kernel
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
// Athena currently not supporting dynamic linking
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


#ifdef ATN_ASSERTS
	#define ATN_ASSERT(x, ...)	{ if(!(x)) { ATN_ERROR("Assertion Failed: {0}", __VA_ARGS__); ATN_DEBUGBREAK(); } }
	#define ATN_CORE_ASSERT(x, ...)  { if(!(x)) { ATN_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); ATN_DEBUGBREAK(); } }
#else
	#define ATN_ASSERT(x, ...) 
	#define ATN_CORE_ASSERT(x, ...)
#endif


#define BIT(x) (1 << x)

#define ATN_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }


namespace Athena
{
	using int8  = ::std::int8_t;  // 8-bit int
	using int16 = ::std::int16_t; // 16-bit int
	using int32 = ::std::int32_t; // 32-bit int
	using int64 = ::std::int64_t; // 64-bit int

	using uint8  = ::std::uint8_t;  // 8-bit unsigned int
	using uint16 = ::std::uint16_t; // 16-bit unsigned int
	using uint32 = ::std::uint32_t; // 32-bit unsigned int
	using uint64 = ::std::uint64_t; // 64-bit unsigned int
	 
	using byte = ::std::byte; // type of size 1 byte
	using String = std::string; // string type
	using SIZE_T = uint64; // size type, same size as a pointer

	using RendererID = uint32; // type for Renderer IDs


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
