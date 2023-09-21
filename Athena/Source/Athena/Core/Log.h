#pragma once

#include "Athena/Core/Core.h"


#if defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
//#include <spdlog/fmt/bundled/format.h>

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif


namespace Athena
{
	class ATHENA_API Log
	{
	public:
		static void Init(bool createConsole);

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

#define ATN_CORE_TRACE(...)		::Athena::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define ATN_CORE_INFO(...)	    ::Athena::Log::GetCoreLogger()->info(__VA_ARGS__)
#define ATN_CORE_WARN(...)      ::Athena::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define ATN_CORE_ERROR(...)     ::Athena::Log::GetCoreLogger()->error(__VA_ARGS__)
#define ATN_CORE_FATAL(...)     ::Athena::Log::GetCoreLogger()->critical(__VA_ARGS__)


//Client log macros

#define ATN_TRACE(...)		  ::Athena::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ATN_INFO(...)	      ::Athena::Log::GetClientLogger()->info(__VA_ARGS__)
#define ATN_WARN(...)         ::Athena::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ATN_ERROR(...)        ::Athena::Log::GetClientLogger()->error(__VA_ARGS__)
#define ATN_FATAL(...)        ::Athena::Log::GetClientLogger()->critical(__VA_ARGS__)


// Example with tag: ATN_TRACE_TAG(Editor", "Fatal error") - "[Editor] - Fatal error"
// Core logs with tag

#define ATN_CORE_TRACE_TAG(tag, msg) ATN_CORE_TRACE("[" tag "] - " msg)
#define ATN_CORE_INFO_TAG(tag, msg)  ATN_CORE_INFO("[" tag "] - " msg)
#define ATN_CORE_WARN_TAG(tag, msg)  ATN_CORE_WARN("[" tag "] - " msg)
#define ATN_CORE_ERROR_TAG(tag, msg) ATN_CORE_ERROR("[" tag "] - " msg)
#define ATN_CORE_FATAL_TAG(tag, msg) ATN_CORE_FATAL("[" tag "] - " msg)

// Client logs with tag

#define ATN_TRACE_TAG(tag, msg) ATN_TRACE("[" tag "] - " msg)
#define ATN_INFO_TAG(tag, msg)  ATN_INFO("[" tag "] - " msg)
#define ATN_WARN_TAG(tag, msg)  ATN_WARN("[" tag "] - " msg)
#define ATN_ERROR_TAG(tag, msg) ATN_ERROR("[" tag "] - " msg)
#define ATN_FATAL_TAG(tag, msg) ATN_FATAL("[" tag "] - " msg)

// TODO: remove these macros
// Log with tag with format args

#define ATN_CORE_TRACE_TAG_(tag, msg, ...) ATN_CORE_TRACE("[" tag "] - " msg, __VA_ARGS__)
#define ATN_CORE_INFO_TAG_(tag, msg, ...) ATN_CORE_INFO("[" tag "] - " msg, __VA_ARGS__)
#define ATN_CORE_WARN_TAG_(tag, msg, ...) ATN_CORE_WARN("[" tag "] - " msg, __VA_ARGS__)
#define ATN_CORE_ERROR_TAG_(tag, msg, ...) ATN_CORE_ERROR("[" tag "] - " msg, __VA_ARGS__)
#define ATN_CORE_FATAL_TAG_(tag, msg, ...) ATN_CORE_FATAL("[" tag "] - " msg, __VA_ARGS__)

#define ATN_TRACE_TAG_(tag, msg, ...) ATN_TRACE("[" tag "] - " msg, __VA_ARGS__)
#define ATN_INFO_TAG_(tag, msg, ...) ATN_INFO("[" tag "] - " msg, __VA_ARGS__)
#define ATN_WARN_TAG_(tag, msg, ...) ATN_WARN("[" tag "] - " msg, __VA_ARGS__)
#define ATN_ERROR_TAG_(tag, msg, ...) ATN_ERROR("[" tag "] - " msg, __VA_ARGS__)
#define ATN_FATAL_TAG_(tag, msg, ...) ATN_FATAL("[" tag "] - " msg, __VA_ARGS__)
