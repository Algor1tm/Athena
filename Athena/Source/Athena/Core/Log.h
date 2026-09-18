#pragma once

#include "Athena/Core/Core.h"

#include <string_view>

#if defined(_MSC_VER)
	#pragma warning(push, 0)
#endif

#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif


namespace Athena
{
	enum class LogLevel
	{
		Trace = 1, Info, Warn, Error, Fatal
	};

	struct LogConfig
	{
		bool EnableConsole = true;
		FilePath OutputPath;	// Relative to working dir
	};


	struct LogCategory
	{
		LogCategory(::std::string_view CategoryName)
			: Name(CategoryName)
		{}

		::std::string_view Name;
		bool Enabled = true;
	};


	class ATHENA_API Logger
	{
	public:
		static Logger& Get()
		{
			return s_Instance;
		}

		void Init(const LogConfig& config);
		void Shutdown();

		template <typename... Args>
		void Message(const LogCategory& category, LogLevel level, Args&&... args);

	private:
		inline String FormatMessage(const String& msg);

		template <typename... Args>
		inline String FormatMessage(const String& msg, Args&&... args);

		void MessageInternal(const String& message, LogLevel level);

	private:
		static Logger s_Instance;
	};



	template <typename... Args>
	void Logger::Message(const LogCategory& category, LogLevel level, Args&&... args)
	{
		if (!category.Enabled)
			return;

		std::string_view logTemplate = "[{0}] {1}";

		String msg = FormatMessage(args...);
		String finalMsg = fmt::vformat(logTemplate, fmt::make_format_args(category.Name, msg));

		MessageInternal(finalMsg, level);
	}

	inline String Logger::FormatMessage(const String& msg)
	{
		return msg;
	}

	template <typename... Args>
	inline String Logger::FormatMessage(const String& msg, Args&&... args)
	{
		String formattedMsg = fmt::vformat(msg, fmt::make_format_args(args...));
		return formattedMsg;
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// If defined template specialization of custom type 'Type' ToString<Type>, 
// objects of class 'Type' can be formatted
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace Athena
{
	template <typename T>
	inline String ToString(const T& x)
	{
		return "Unknown type!";
	}

	template<>
	inline String ToString<std::wstring>(const std::wstring& x)
	{
		return FilePath(x).string();
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


#if ATN_ENABLE_LOGGING
	#define LOG_CAT_SYMBOL_NAME(CategoryName) g_LogCategory_##CategoryName

	#define EXPORT_LOG_CATEGORY(CategoryName) extern ATHENA_API LogCategory LOG_CAT_SYMBOL_NAME(CategoryName)
	#define DEFINE_LOG_CATEGORY(CategoryName) ATHENA_API LogCategory LOG_CAT_SYMBOL_NAME(CategoryName) = LogCategory(ATN_STRINGIFY_MACRO(CategoryName))

	#define ATN_LOG_TRACE(CategoryName, ...) ::Athena::Logger::Get().Message( ::Athena::LOG_CAT_SYMBOL_NAME(CategoryName), ::Athena::LogLevel::Trace, __VA_ARGS__)
	#define ATN_LOG_INFO(CategoryName, ...)  ::Athena::Logger::Get().Message( ::Athena::LOG_CAT_SYMBOL_NAME(CategoryName), ::Athena::LogLevel::Info, __VA_ARGS__)
	#define ATN_LOG_WARN(CategoryName, ...)  ::Athena::Logger::Get().Message( ::Athena::LOG_CAT_SYMBOL_NAME(CategoryName), ::Athena::LogLevel::Warn, __VA_ARGS__)
	#define ATN_LOG_ERROR(CategoryName, ...) ::Athena::Logger::Get().Message( ::Athena::LOG_CAT_SYMBOL_NAME(CategoryName), ::Athena::LogLevel::Error, __VA_ARGS__);
	#define ATN_LOG_FATAL(CategoryName, ...) ::Athena::Logger::Get().Message( ::Athena::LOG_CAT_SYMBOL_NAME(CategoryName), ::Athena::LogLevel::Fatal, __VA_ARGS__); ATN_DEBUGBREAK()
#else
	#define EXPORT_LOG_CATEGORY(CategoryName)
	#define DEFINE_LOG_CATEGORY(CategoryName)

	#define ATN_LOG_TRACE(CategoryName, ...)
	#define ATN_LOG_INFO(CategoryName, ...)
	#define ATN_LOG_WARN(CategoryName, ...)
	#define ATN_LOG_ERROR(CategoryName, ...)
	#define ATN_LOG_FATAL(CategoryName, ...)
#endif

namespace Athena
{
	EXPORT_LOG_CATEGORY(LogTemp);
	EXPORT_LOG_CATEGORY(Debug);
	EXPORT_LOG_CATEGORY(General);
	EXPORT_LOG_CATEGORY(Windows);
	EXPORT_LOG_CATEGORY(Renderer);
	EXPORT_LOG_CATEGORY(Vulkan);
	EXPORT_LOG_CATEGORY(AssetManager);
	EXPORT_LOG_CATEGORY(FileSystem);
	EXPORT_LOG_CATEGORY(Scene);
	EXPORT_LOG_CATEGORY(ScriptEngine);
	EXPORT_LOG_CATEGORY(Editor);
}
