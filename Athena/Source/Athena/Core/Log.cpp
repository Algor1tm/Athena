#include "Log.h"

#if defined(_MSC_VER)
	#pragma warning(push, 0)
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


static void CreateConsole()
{
	int hConHandle;
	__int64 lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE* fp;

	const WORD CONSOLE_LINES = 1000;

	// allocate a console for this app
	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);

	coninfo.dwSize.Y = CONSOLE_LINES;
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

static void CreateConsole()
{
	ATN_CORE_VERIFY(false, "Not implemented for this platform");
}

#endif

namespace Athena
{
	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;


	void Log::Init(bool createConsole)
	{
		if(createConsole)
			CreateConsole();

		spdlog::set_pattern("%^[%T] %n: %v%$");

		std::vector<spdlog::sink_ptr> logSinks;

		logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Athena.log", true));
		logSinks[0]->set_pattern("[%T] [%l] %n: %v");

		if (createConsole)
		{
			logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
			logSinks[1]->set_pattern("%^[%T] %n: %v%$");
		}

		spdlog::level::level_enum loglevel;
#ifdef ATN_LOG_LEVEL_DEBUG
		loglevel = spdlog::level::trace;
#else
		loglevel = spdlog::level::info;
#endif

		s_CoreLogger = std::make_shared<spdlog::logger>("ATHENA", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_CoreLogger);
		s_CoreLogger->set_level(loglevel);
		s_CoreLogger->flush_on(loglevel);

		s_ClientLogger = std::make_shared<spdlog::logger>("APP", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_ClientLogger);
		s_ClientLogger->set_level(loglevel);
		s_ClientLogger->flush_on(loglevel);
	}
}
