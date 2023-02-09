#pragma once

#include "Core.h"


#if defined(_MSC_VER)
	#pragma warning (push)
	#pragma warning( disable: 4996 )
	#pragma warning( disable: 26451 )
	#pragma warning( disable: 6285 )
	#pragma warning( disable: 26437 )
	#pragma warning( disable: 26115 )
	#pragma warning( disable: 26498 )
	#pragma warning( disable: 26800 )
	#pragma warning( disable: 26495 )
#endif

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#if defined(_MSC_VER)
	#pragma warning( pop)
#endif



namespace Athena
{
	class ATHENA_API Log
	{
	public:
		static void Init();
		static void InitWithoutConsole();

		static void Disable();
		static void Enable();

		inline static const Ref<spdlog::logger>& GetCoreLogger()   { return s_CoreLogger; }
		inline static const Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};


	template <typename T>
	constexpr const char* ToString(const T& x)
	{
		return "Unknown type!";
	}

	constexpr const String& ToString(const String& x)
	{
		return x;
	}

	inline std::string ToString(const std::wstring& x)
	{
		return FilePath(x).string();
	}

	constexpr const char* ToString(const char* x)
	{
		return x;
	}
}


namespace fmt
{
	template <typename OStream, typename T>
	inline OStream& operator<<(OStream& os, const T& x)
	{
		return os << ::Athena::ToString(x);
	}
}


//Core log macros
#define ATN_CORE_TRACE(...)   ::Athena::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ATN_CORE_INFO(...)	  ::Athena::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ATN_CORE_WARN(...)    ::Athena::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ATN_CORE_ERROR(...)   ::Athena::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ATN_CORE_FATAL(...)   ::Athena::Log::GetCoreLogger()->critical(__VA_ARGS__)

//Client log macros
#define ATN_TRACE(...)        ::Athena::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ATN_INFO(...)	      ::Athena::Log::GetClientLogger()->info(__VA_ARGS__)
#define ATN_WARN(...)         ::Athena::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ATN_ERROR(...)        ::Athena::Log::GetClientLogger()->error(__VA_ARGS__)
#define ATN_FATAL(...)        ::Athena::Log::GetClientLogger()->critical(__VA_ARGS__)
