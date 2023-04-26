#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Scripting/ScriptFields.h"


namespace Athena
{
	class Entity;

	struct ScriptConfig
	{
		FilePath ScriptsFolder;
	};

	// If include PrivateScriptEngine.h pybind will try to link Python.dll
	class ATHENA_API ScriptEngine
	{
	public:
		static void Init(const ScriptConfig& config);
		static void Shutdown();

		static ScriptFieldMap& GetScriptFieldMap(Entity entity);
		static const ScriptFieldsDescription& GetFieldsDescription(const String& className);

		static void LoadScript(const String& name, Entity entity);
		static void UnLoadScript(const String& name, Entity entity);
		static bool ExistsScript(const String& name);

		static std::vector<String> GetAvailableModules();
	};
}
