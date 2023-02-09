#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Scripting/ScriptFields.h"


namespace Athena
{
	class Entity;

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
