#include "Log.h"

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

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif


#ifdef ATN_PLATFORM_WINDOWS
#include <Windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <fstream>

// maximum mumber of lines the output console should have
static const WORD MAX_CONSOLE_LINES = 500;

static void RedirectIOToConsole()
{
	int hConHandle;
	__int64 lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE* fp;

	// allocate a console for this app
	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// redirect unbuffered STDOUT to the console
	lStdHandle = reinterpret_cast<__int64>(GetStdHandle(STD_OUTPUT_HANDLE));
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);

	std::ios::sync_with_stdio();
}

#else

static void RedirectIOToConsole()
{

}

#endif

namespace Athena
{
	Ref<spdlog::logger> Log::s_CoreLogger;
	Ref<spdlog::logger> Log::s_ClientLogger;


	void Log::Init()
	{
		RedirectIOToConsole();

		spdlog::set_pattern("%^[%T] %n: %v%$");

		std::vector<spdlog::sink_ptr> logSinks;
		logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Athena.log", true));

		logSinks[0]->set_pattern("%^[%T] %n: %v%$");
		logSinks[1]->set_pattern("[%T] [%l] %n: %v");

		s_CoreLogger = CreateRef<spdlog::logger>("ATHENA", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_CoreLogger);
		s_CoreLogger->set_level(spdlog::level::trace);
		s_CoreLogger->flush_on(spdlog::level::trace);

		s_ClientLogger = CreateRef<spdlog::logger>("APP", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_ClientLogger);
		s_ClientLogger->set_level(spdlog::level::trace);
		s_ClientLogger->flush_on(spdlog::level::trace);
	}

	void Log::InitWithoutConsole()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");

		spdlog::sink_ptr logSinks;
		logSinks = std::make_shared<spdlog::sinks::basic_file_sink_mt>("Athena.log", true);

		logSinks->set_pattern("[%T] [%l] %n: %v");

		s_CoreLogger = CreateRef<spdlog::logger>("ATHENA", logSinks);
		spdlog::register_logger(s_CoreLogger);
		s_CoreLogger->set_level(spdlog::level::trace);
		s_CoreLogger->flush_on(spdlog::level::trace);

		s_ClientLogger = CreateRef<spdlog::logger>("APP", logSinks);
		spdlog::register_logger(s_ClientLogger);
		s_ClientLogger->set_level(spdlog::level::trace);
		s_ClientLogger->flush_on(spdlog::level::trace);
	}

	void Log::Disable()
	{
		s_CoreLogger->set_level(spdlog::level::off);
		s_CoreLogger->flush_on(spdlog::level::off);

		s_ClientLogger->set_level(spdlog::level::off);
		s_ClientLogger->flush_on(spdlog::level::off);
	}

	void Log::Enable()
	{
		s_CoreLogger->set_level(spdlog::level::trace);
		s_CoreLogger->flush_on(spdlog::level::trace);

		s_ClientLogger->set_level(spdlog::level::trace);
		s_ClientLogger->flush_on(spdlog::level::trace);
	}
}
