#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Scene.h"
#include "Athena/Scripting/ScriptFields.h"


namespace Athena
{
	struct ScriptConfig
	{
		FilePath ScriptsPath;
		FilePath ModulePath;
		bool EnableDebug;
	};

	// If include PrivateScriptEngine.h pybind will try to link Python.dll
	class ATHENA_API ScriptEngine
	{
	public:
		static void Init(const ScriptConfig& config);
		static void Shutdown();

		static void OnRuntimeStart(Scene* scene);

		static void OnUpdateEntity(Entity entity, Time frameTime);
		static void InstantiateEntity(Entity entity);
		static void OnCreateEntity(Entity entity);
	};
}
