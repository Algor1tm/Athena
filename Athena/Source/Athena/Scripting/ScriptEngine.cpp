#include "ScriptEngine.h"

#include "Athena/Scripting/PrivateScriptEngine.h"


namespace Athena
{
	void ScriptEngine::Init(const ScriptConfig& config)
	{
		PrivateScriptEngine::Init(config);
	}

	void ScriptEngine::Shutdown()
	{
		PrivateScriptEngine::Shutdown();
	}

	const ScriptFieldsDescription& ScriptEngine::GetFieldsDescription(const String& className)
	{
		const auto& scriptClass = PrivateScriptEngine::GetScriptClass(className);
		return scriptClass.GetFieldsDescription();
	}

	ScriptFieldMap& ScriptEngine::GetScriptFieldMap(Entity entity)
	{
		return PrivateScriptEngine::GetScriptFieldMap(entity);
	}

	void ScriptEngine::LoadScript(const String& name, Entity entity)
	{
		PrivateScriptEngine::LoadScript(name, entity);
	}

	void ScriptEngine::UnLoadScript(const String& name, Entity entity)
	{
		PrivateScriptEngine::UnloadScript(name, entity);
	}

	bool ScriptEngine::ExistsScript(const String& name)
	{
		return PrivateScriptEngine::ExistsScript(name);
	}

	std::vector<String> ScriptEngine::GetAvailableModules()
	{
		return PrivateScriptEngine::GetAvailableModules();
	}
}
