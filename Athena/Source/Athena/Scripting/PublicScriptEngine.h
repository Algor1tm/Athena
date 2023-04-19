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

	// TODO: ScriptEngine -> PrivateScriptEngine, PublicScriptEngine -> ScriptEngine
	// If include ScriptEngine.h pybind will try to link Python.dll
	class ATHENA_API PublicScriptEngine
	{
	public:
		static ScriptFieldMap& GetScriptFieldMap(Entity entity);
		static void ReloadScripts();
		static const ScriptFieldsDescription& GetFieldsDescription(const String& className);
		static void OnScriptComponentRemove(Entity entity);
		static std::vector<String> GetAvailableModules();
	};
}
