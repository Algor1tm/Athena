#pragma once

#include "spdlog/spdlog.h"

#include "Core.h"


namespace Athena
{
	class ATHENA_API Log
	{
	public:
		static void Init();

		inline static const std::shared_ptr<spdlog::logger>& GetCoreLogger()   { return s_CoreLogger; }
		inline static const std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
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
