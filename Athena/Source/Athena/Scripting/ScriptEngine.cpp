#include "ScriptEngine.h"


#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(push, 0)
#endif

#include <pybind11/embed.h>

#ifdef _MSC_VER
#undef _CRT_SECURE_NO_WARNINGS
#pragma warning(pop)
#endif

#include <unordered_map>

namespace py = pybind11;


namespace Athena
{
	struct ScriptEngineData
	{
		py::scoped_interpreter PythonInterpreter;
		Filepath ScriptsFolder;

		std::unordered_map<String, py::module_> PythonModules;

		std::unordered_map<String, ScriptClass> EntityClasses;
		std::unordered_map<UUID, ScriptInstance> EntityInstances;
		Scene* SceneContext = nullptr;
	};

	static ScriptEngineData* s_Data;

	static std::vector<String> GetStringPyModules()
	{
		std::vector<String> modules;

		for (const auto& dirEntry : std::filesystem::directory_iterator(s_Data->ScriptsFolder))
		{
			if (!dirEntry.is_directory())
			{
				const auto& path = dirEntry.path();
				if (path.extension() == L".py")
				{
					const auto& filename = path.stem().string();
					if (filename.find("Athena") == std::string::npos)
					{
						modules.push_back(filename);
					}
				}
			}
		}

		return modules;
	}

	static void ImportAndAddModule(const String& name)
	{
		py::module_ check = s_Data->PythonModules[name] = py::module_::import(name.c_str());
		if (check)
			ATN_CORE_INFO("Successfuly load python module '{0}{1}'", name, ".py");
		else
			ATN_CORE_FATAL("Failed to load python module '{0}{1}' !", name, ".py");
	}

	ScriptClass::ScriptClass(const String& className)
	{
		py::module_ pyModule = s_Data->PythonModules.at(className);

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



	void ScriptEngine::Init(const Filepath& scriptsFolder)
	{
		s_Data = new ScriptEngineData();
		s_Data->ScriptsFolder = scriptsFolder;

		py::module_ sys = py::module_::import("sys");
		auto& path = sys.attr("path");
		path.attr("insert")(0, s_Data->ScriptsFolder.string().c_str());

		const auto& modules = GetStringPyModules();
		for (const auto& strModule : modules)
		{
			ImportAndAddModule(strModule);
			s_Data->EntityClasses[strModule] = ScriptClass(strModule);
		}
	}

	void ScriptEngine::ReloadScripts()
	{
		const auto& modules = GetStringPyModules();

		for (const String& strModule : modules)
		{
			if (s_Data->PythonModules.find(strModule) == s_Data->PythonModules.end())
			{
				ImportAndAddModule(strModule);
			}
			else
			{
				s_Data->PythonModules.at(strModule).reload();
				ATN_CORE_INFO("Reload python module {0}{1}", strModule,".py");
			}
		}

		s_Data->EntityClasses.clear();
		for (const auto& [moduleName, _] : s_Data->PythonModules)
		{
			s_Data->EntityClasses[moduleName] = ScriptClass(moduleName);
		}
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
		s_Data->EntityInstances.clear();
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
