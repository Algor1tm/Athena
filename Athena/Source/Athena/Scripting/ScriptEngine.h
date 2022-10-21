#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Entity.h"

#ifdef _MSC_VER
	#pragma warning(push, 0)
#endif

#include <pybind11/embed.h>

#ifdef _MSC_VER
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

		py::object Instantiate();
		py::object GetMethod(const String& name);

	private:
		py::object m_PyClass;
	};

	class ScriptInstance
	{
	public:
		ScriptInstance() = default;
		ScriptInstance(ScriptClass scriptClass, Entity entity);

		void InvokeOnCreate();
		void InvokeOnUpdate(Time frameTime);

	private:
		ScriptClass m_ScriptClass;
		py::object m_OnCreateMethod;
		py::object m_OnUpdateMethod;

		py::object m_PyInstance;
	};


	class ATHENA_API ScriptEngine
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnRuntimeStart(Scene* scene);
		static void OnRuntimeStop();

		static Scene* GetSceneContext();
		static bool EntityClassExists(const String& name);

		static void OnCreateEntity(Entity entity);
		static void OnUpdateEntity(Entity entity, Time frameTime);
	};
}
