#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Entity.h"

#include "Athena/Scripting/ScriptFields.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(push, 0)
#endif

#include <pybind11/pytypes.h>

#ifdef _MSC_VER
#undef _CRT_SECURE_NO_WARNINGS
#pragma warning(pop)
#endif

namespace py = pybind11;


namespace Athena
{
	class ScriptClass
	{
	public:
		ScriptClass() = default;
		ScriptClass(const String& className);

		py::object Instantiate(Entity entity);
		py::object GetMethod(const String& name);

		py::object GetOnCreateMethod() { return m_OnCreateMethod; }
		py::object GetOnUpdateMethod() { return m_OnUpdateMethod; }

		py::object GetInternalClass() { return m_PyClass; }
		bool IsLoaded() const { return m_IsLoaded; }

		const ScriptFieldsDescription& GetFieldsDescription() const { return m_FieldsDescription; }

	private:
		py::object m_PyClass;
		py::object m_OnCreateMethod;
		py::object m_OnUpdateMethod;

		ScriptFieldsDescription m_FieldsDescription;
		bool m_IsLoaded = false;
	};

	class ScriptInstance
	{
	public:
		ScriptInstance() = default;
		ScriptInstance(ScriptClass scriptClass, Entity entity);

		void InvokeOnCreate();
		void InvokeOnUpdate(Time frameTime);

		py::object GetInternalInstance();

	private:
		ScriptClass m_ScriptClass;
		py::object m_PyInstance;
	};

	struct ScriptConfig;

	class PrivateScriptEngine
	{
	public:
		static void Init(const ScriptConfig& config);
		static void Shutdown();

		static void LoadScript(const String& name, Entity entity);
		static void UnloadScript(const String& name, Entity entity);
		static void ReloadScript(const String& name, Entity entity);
		static bool ExistsScript(const String& name);

		static void OnRuntimeStart(Scene* scene);
		static void OnRuntimeStop();

		static ScriptFieldMap& GetScriptFieldMap(Entity entity);

		static Scene* GetSceneContext();
		static bool EntityClassExists(const String& name);
		static bool EntityInstanceExists(UUID uuid);

		static ScriptInstance& GetScriptInstance(UUID uuid);
		static const ScriptClass& GetScriptClass(const String& name);

		static void InstantiateEntity(Entity entity);
		static void OnCreateEntity(Entity entity);
		static void OnUpdateEntity(Entity entity, Time frameTime);

		static std::vector<String> GetAvailableModules();
	};
}
