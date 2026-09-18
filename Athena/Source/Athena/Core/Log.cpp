#include "Log.h"
#include "Athena/Project/Project.h"
#include "Athena/Core/PlatformUtils.h"

#if defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif



namespace Athena
{
	DEFINE_LOG_CATEGORY(LogTemp);
	DEFINE_LOG_CATEGORY(Debug);
	DEFINE_LOG_CATEGORY(General);
	DEFINE_LOG_CATEGORY(Windows);
	DEFINE_LOG_CATEGORY(Renderer);
	DEFINE_LOG_CATEGORY(Vulkan);
	DEFINE_LOG_CATEGORY(AssetManager);
	DEFINE_LOG_CATEGORY(FileSystem);
	DEFINE_LOG_CATEGORY(Scene);
	DEFINE_LOG_CATEGORY(ScriptEngine);
	DEFINE_LOG_CATEGORY(Editor);

	Logger Logger::s_Instance;
	static std::shared_ptr<spdlog::logger> s_SPDLogger;

	static std::string_view LogLevelToString(LogLevel level)
	{
		switch (level)
		{
		case LogLevel::Trace: return "Trace";
		case LogLevel::Info:  return "Info";
		case LogLevel::Warn:  return "Warn";
		case LogLevel::Error: return "Error";
		case LogLevel::Fatal: return "Fatal";
		}

		return "";
	}

	void Logger::Init(const LogConfig& config)
	{
		if(config.EnableConsole)
			Platform::CreateAndSyncConsole();

		spdlog::set_pattern("%^[%T] %n: %v%$");

		std::vector<spdlog::sink_ptr> logSinks;

		logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(config.OutputPath.string(), true));
		logSinks[0]->set_pattern("[%T] [%l] %n: %v");

		if (config.EnableConsole)
		{
			logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
			logSinks[1]->set_pattern("%^[%T] %n: %v%$");
		}

		spdlog::level::level_enum loglevel;

#if ATN_DEBUG
		loglevel = spdlog::level::trace;
#else
		loglevel = spdlog::level::info;
#endif

		s_SPDLogger = std::make_shared<spdlog::logger>("ATHENA", begin(logSinks), end(logSinks));
		spdlog::register_logger(s_SPDLogger);
		s_SPDLogger->set_level(loglevel);
		s_SPDLogger->flush_on(loglevel);
	}

	void Logger::Shutdown()
	{
		s_SPDLogger.reset();
	}

	void Logger::MessageInternal(const String& message, LogLevel level)
	{
		switch (level)
		{
		case LogLevel::Trace:
			s_SPDLogger->trace(message);
			break;
		case LogLevel::Info:
			s_SPDLogger->info(message);
			break;
		case LogLevel::Warn:
			s_SPDLogger->warn(message);
			break;
		case LogLevel::Error:
			s_SPDLogger->error(message);
			break;
		case LogLevel::Fatal:
			s_SPDLogger->critical(message);
			break;
		}

		String formattedMessage = FormatMessage("[{}] ATHENA: {}\n", LogLevelToString(level), message);
		Platform::LogNative(formattedMessage);
	}
}
