#include "PublicScriptEngine.h"

#include "ScriptEngine.h"


namespace Athena
{
	const ScriptFieldsDescription& PublicScriptEngine::GetFieldsDescription(const String& className)
	{
		const auto& scriptClass = ScriptEngine::GetScriptClass(className);
		return scriptClass.GetFieldsDescription();
	}

	ScriptFieldMap& PublicScriptEngine::GetScriptFieldMap(Entity entity)
	{
		return ScriptEngine::GetScriptFieldMap(entity);
	}

	void PublicScriptEngine::ReloadScripts()
	{
		ScriptEngine::ReloadScripts();
	}
}
