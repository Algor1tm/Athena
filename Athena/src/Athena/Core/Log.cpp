#include "atnpch.h"
#include "Log.h"

#if defined(_MSC_VER)
	#pragma warning (push, 0)
#endif

#include <spdlog/sinks/stdout_color_sinks.h>

#if defined(_MSC_VER)
	#pragma warning( pop)
#endif

namespace Athena
{
	Ref<spdlog::logger> Log::s_CoreLogger;
	Ref<spdlog::logger> Log::s_ClientLogger;


	void Log::Init()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");

		s_CoreLogger = spdlog::stdout_color_mt("ATHENA");
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = spdlog::stdout_color_mt("APP");
		s_CoreLogger->set_level(spdlog::level::trace);
	}
}
