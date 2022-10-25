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
		ATN_CORE_ASSERT(m_PyClass, "Failed to load script class!");
	}

	py::object ScriptClass::Instantiate(Entity entity)
	{
		return m_PyClass(py::cast((UUID)(entity.GetID())));
	}

	py::object ScriptClass::GetMethod(const String& name)
	{
		py::object method = m_PyClass.attr(name.data());
		ATN_CORE_ASSERT(method, "Failed to load method!");
		return method;
	}



	ScriptInstance::ScriptInstance(ScriptClass scriptClass, Entity entity)
		: m_ScriptClass(scriptClass)
	{
		m_PyInstance = m_ScriptClass.Instantiate(entity);

		m_OnCreateMethod = m_ScriptClass.GetMethod("OnCreate");
		m_OnUpdateMethod = m_ScriptClass.GetMethod("OnUpdate");
	}

	void ScriptInstance::InvokeOnCreate()
	{
		m_OnCreateMethod(m_PyInstance);

		py::dict fields = py::cast<py::dict>(m_ScriptClass.GetInternalClass().attr("__dict__"));
		for (const auto& [nameHandle, _] : fields)
		{
			std::string name = py::cast<std::string>(nameHandle);
			if (name.substr(0, 2) != "__")
			{
				std::string type = py::cast<std::string>(m_PyInstance.attr(name.c_str()).attr("__class__").attr("__name__"));
				if (type != "method")
				{
					ATN_CORE_WARN("Field: {0}, Type: {1}", name, type);
				}
			}
		}
		ATN_CORE_WARN("");
	}

	void ScriptInstance::InvokeOnUpdate(Time frameTime)
	{
		m_OnUpdateMethod(m_PyInstance, frameTime);
	}

	py::object ScriptInstance::GetInternalInstance()
	{
		return m_PyInstance;
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
		s_Data->SceneContext = nullptr;
	}

	Scene* ScriptEngine::GetSceneContext()
	{
		return s_Data->SceneContext;
	}

	bool ScriptEngine::EntityClassExists(const String& name)
	{
		return s_Data->EntityClasses.find(name) != s_Data->EntityClasses.end();
	}

	bool ScriptEngine::EntityInstanceExists(UUID uuid)
	{
		return s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end();
	}

	ScriptInstance& ScriptEngine::GetScriptInstance(UUID uuid)
	{
		ATN_CORE_ASSERT(s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end());
		return s_Data->EntityInstances.at(uuid);
	}

	void ScriptEngine::InstantiateEntity(Entity entity)
	{
		auto& sc = entity.GetComponent<ScriptComponent>();

		if (!EntityClassExists(sc.Name))
		{
			s_Data->EntityClasses[sc.Name] = ScriptClass(sc.Name);
		}

		auto& instance = s_Data->EntityInstances[entity.GetID()] = ScriptInstance(s_Data->EntityClasses.at(sc.Name), entity);
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		s_Data->EntityInstances[entity.GetID()].InvokeOnCreate();
	}

	void ScriptEngine::OnUpdateEntity(Entity entity, Time frameTime)
	{
		s_Data->EntityInstances[entity.GetID()].InvokeOnUpdate(frameTime);
	}
}
