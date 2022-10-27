#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Entity.h"

#include "ScriptFields.h"


namespace Athena
{
	class ATHENA_API PublicScriptEngine
	{
	public:
		static ScriptFieldMap& GetScriptFieldMap(Entity entity);
		static void ReloadScripts();
		static const ScriptFieldsDescription& GetFieldsDescription(const String& className);
	};
}
