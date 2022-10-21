#include "ScriptEngine.h"

#include <unordered_map>


namespace Athena
{
	struct ScriptEngineData
	{
		py::scoped_interpreter PythonInterpreter;

		std::unordered_map<String, ScriptClass> EntityClasses;
		std::unordered_map<UUID, ScriptInstance> EntityInstances;
		Scene* SceneContext = nullptr;
	};

	static ScriptEngineData* s_Data;


	ScriptClass::ScriptClass(const String& className)
	{
		py::module_ pyModule = py::module_::import(className.data());
		ATN_CORE_ASSERT(pyModule, "Failed to load script!");

		m_PyClass = pyModule.attr(className.data());
	}

	py::object ScriptClass::Instantiate()
	{
		return m_PyClass();
	}

	py::object ScriptClass::GetMethod(const String& name)
	{
		return m_PyClass.attr(name.data());
	}



	ScriptInstance::ScriptInstance(ScriptClass scriptClass, Entity entity)
		: m_ScriptClass(scriptClass)
	{
		m_PyInstance = m_ScriptClass.Instantiate();
		m_PyInstance.attr("_ID") = py::cast((uint64)(entity.GetID()));

		m_OnCreateMethod = m_ScriptClass.GetMethod("OnCreate");
		m_OnUpdateMethod = m_ScriptClass.GetMethod("OnUpdate");
	}

	void ScriptInstance::InvokeOnCreate()
	{
		m_OnCreateMethod(m_PyInstance);
	}

	void ScriptInstance::InvokeOnUpdate(Time frameTime)
	{
		m_OnUpdateMethod(m_PyInstance, frameTime);
	}



	void ScriptEngine::Init()
	{
		s_Data = new ScriptEngineData();

		py::module_ sys = py::module_::import("sys");
		auto& path = sys.attr("path");
		path.attr("insert")(0, "Assets/Scripts/");
	}

	void ScriptEngine::Shutdown()
	{
		delete s_Data;
	}

	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		s_Data->SceneContext = scene;
	}

	void ScriptEngine::OnRuntimeStop()
	{

	}

	Scene* ScriptEngine::GetSceneContext()
	{
		return s_Data->SceneContext;
	}

	bool ScriptEngine::EntityClassExists(const String& name)
	{
		return s_Data->EntityClasses.find(name) != s_Data->EntityClasses.end();
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		auto& sc = entity.GetComponent<ScriptComponent>();

		if (!EntityClassExists(sc.Name))
		{
			s_Data->EntityClasses[sc.Name] = ScriptClass(sc.Name);
		}

		auto& instance = s_Data->EntityInstances[entity.GetID()] = ScriptInstance(s_Data->EntityClasses.at(sc.Name), entity);

		instance.InvokeOnCreate();
	}

	void ScriptEngine::OnUpdateEntity(Entity entity, Time frameTime)
	{
		s_Data->EntityInstances[entity.GetID()].InvokeOnUpdate(frameTime);
	}
}
