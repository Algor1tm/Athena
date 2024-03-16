#include "Athena/Scripting/ScriptEngine.h"
#include "Athena/Scripting/PrivateScriptEngine.h"

#include "Athena/Core/FileSystem.h"

#include "Athena/Scene/Components.h"

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
	static void SetInternalValue(py::object accessor, const char* name, ScriptFieldType type, const void* buffer)
	{
		switch (type)
		{
		case ScriptFieldType::Int: accessor.attr(name) = *(int*)buffer; break;
		case ScriptFieldType::Float: accessor.attr(name) = *(float*)buffer; break;
		case ScriptFieldType::Bool: accessor.attr(name) = *(bool*)buffer; break;

		case ScriptFieldType::Vector2: accessor.attr(name) = *(Vector2*)buffer; break;
		case ScriptFieldType::Vector3: accessor.attr(name) = *(Vector3*)buffer; break;
		case ScriptFieldType::Vector4: accessor.attr(name) = *(Vector4*)buffer; break;

		default: ATN_CORE_ASSERT(false);
		}
	}

	static void GetInternalValue(const py::handle& accessor, ScriptFieldType type, void* buffer)
	{
		switch (type)
		{
		case ScriptFieldType::Int: { int value = py::cast<int>(accessor); memcpy(buffer, &value, sizeof(value)); break; }
		case ScriptFieldType::Float: { float value = py::cast<float>(accessor); memcpy(buffer, &value, sizeof(value)); break; }
		case ScriptFieldType::Bool: { bool value = py::cast<bool>(accessor); memcpy(buffer, &value, sizeof(value)); break; }

		case ScriptFieldType::Vector2: { Vector2 value = py::cast<Vector2>(accessor); memcpy(buffer, &value, sizeof(value)); break; }
		case ScriptFieldType::Vector3: { Vector3 value = py::cast<Vector3>(accessor); memcpy(buffer, &value, sizeof(value)); break; }
		case ScriptFieldType::Vector4: { Vector4 value = py::cast<Vector4>(accessor); memcpy(buffer, &value, sizeof(value)); break; }
									 
		default: ATN_CORE_ASSERT(false);
		}
	}

	static std::unordered_map<String, ScriptFieldType> s_ScriptFieldTypeMap =
	{
		{ "NoneType", ScriptFieldType::None },

		{ "int", ScriptFieldType::Int },
		{ "float", ScriptFieldType::Float },
		{ "bool", ScriptFieldType::Bool },
		{ "str", ScriptFieldType::String },

		{ "Vector2", ScriptFieldType::Vector2 },
		{ "Vector3", ScriptFieldType::Vector3 },
		{ "Vector4", ScriptFieldType::Vector4 }
	};

	struct ScriptEngineData
	{
		py::scoped_interpreter PythonInterpreter;
		FilePath ScriptsFolder;

		std::unordered_map<String, py::module_> PythonModules;

		std::unordered_map<String, ScriptClass> EntityClasses;
		std::unordered_map<UUID, ScriptInstance> EntityInstances;
		std::unordered_map<UUID, ScriptFieldMap> EntityScriptFields;

		Scene* SceneContext = nullptr;
	};

	static ScriptEngineData* s_Data;

	static std::vector<String> GetStringPyModules()
	{
		if (!FileSystem::Exists(s_Data->ScriptsFolder))
			return {};

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

	static bool ImportAndAddModule(const String& name)
	{
		try
		{
			s_Data->PythonModules[name] = py::module_::import(name.c_str());
			ATN_CORE_INFO_TAG("SriptEngine", "Successfully load python script '{}'!", name);
			return true;
		}
		catch (std::exception& exception)
		{
			s_Data->PythonModules.erase(name);
			ATN_CORE_ERROR_TAG("SriptEngine", "Load module exception: \n{}!", exception.what());
			return false;
		}
	}

	static bool ReloadModule(const String& name)
	{
		try
		{
			s_Data->PythonModules.at(name).reload();
			return true;
		}
		catch (std::exception& exception)
		{
			ATN_CORE_ERROR_TAG("SriptEngine", "Reload module exception: \n{}!", exception.what());
			return false;
		}
	}

	static String GetPythonFieldType(const String& name, py::object pyClass)
	{
		return py::cast<std::string>(pyClass.attr(name.c_str()).attr("__class__").attr("__name__"));
	}

	static bool IsValidFieldName(const String& name, py::object pyClass, const String& className)
	{
		if (name.at(0) != '_') // is public
		{
			String type = GetPythonFieldType(name, pyClass);
			if (type != "function")	// is not function
			{
				if (s_ScriptFieldTypeMap.find(type) != s_ScriptFieldTypeMap.end()) // has appropritate c++ type
				{
					return true;
				}
				ATN_CORE_WARN_TAG("SriptEngine", "Unknown field type: {0} ({1}), class - {2}", name, type, className);
			}
		}

		return false;
	}


	ScriptClass::ScriptClass(const String& className)
	{
		py::module_ pyModule = s_Data->PythonModules.at(className);

		try
		{
			m_PyClass = pyModule.attr(className.data());
		}
		catch (std::exception& exception)
		{
			ATN_CORE_ERROR_TAG("SriptEngine", "ScriptClass exception: \n{}!", exception.what());
			return;
		}

		m_IsLoaded = true;

		m_OnCreateMethod = GetMethod("OnCreate");
		m_OnUpdateMethod = GetMethod("OnUpdate");

		py::dict fields = py::cast<py::dict>(m_PyClass.attr("__dict__"));
		for (const auto& [nameHandle, initialValue] : fields)
		{
			std::string name = py::cast<std::string>(nameHandle);
			if (IsValidFieldName(name, m_PyClass, className))
			{
				String type = GetPythonFieldType(name, m_PyClass);

				ScriptField fieldDesc;
				fieldDesc.Name = name;
				fieldDesc.Type = s_ScriptFieldTypeMap.at(type);
				GetInternalValue(initialValue, fieldDesc.Type, fieldDesc.Storage.m_Buffer);

				m_FieldsDescription[name] = fieldDesc;
			}
		}

	}

	py::object ScriptClass::Instantiate(Entity entity)
	{
		return m_PyClass(py::cast((UUID)(entity.GetID())));
	}

	py::object ScriptClass::GetMethod(const String& name)
	{
		try
		{
			if (IsLoaded())
			{
				py::object method = m_PyClass.attr(name.data());
				return method;
			}
		}
		catch (std::exception& exception)
		{
			ATN_CORE_ERROR_TAG("SriptEngine", "ScriptClass exception: \n{}!", exception.what());
		}

		return py::object();
	}



	ScriptInstance::ScriptInstance(ScriptClass scriptClass, Entity entity)
		: m_ScriptClass(scriptClass)
	{
		m_PyInstance = m_ScriptClass.Instantiate(entity);

		UUID entityID = entity.GetID();
		if (s_Data->EntityScriptFields.find(entityID) != s_Data->EntityScriptFields.end())
		{
			const ScriptFieldsDescription& desc = m_ScriptClass.GetFieldsDescription();
			const ScriptFieldMap& fieldMap = s_Data->EntityScriptFields.at(entityID);
			for (const auto& [name, field] : fieldMap)
			{
				auto type = desc.at(name).Type;
				SetInternalValue(m_PyInstance, name.data(), type, field.m_Buffer);
			}
		}
	}

	void ScriptInstance::InvokeOnCreate()
	{
		py::object onCreate = m_ScriptClass.GetOnCreateMethod();
		if (onCreate)
			onCreate(m_PyInstance);
	}

	void ScriptInstance::InvokeOnUpdate(Time frameTime)
	{
		py::object onUpdate = m_ScriptClass.GetOnUpdateMethod();
		if (onUpdate)
			onUpdate(m_PyInstance, frameTime);
	}

	py::object ScriptInstance::GetInternalInstance()
	{
		return m_PyInstance;
	}



	void PrivateScriptEngine::Init(const ScriptConfig& config)
	{
		s_Data = new ScriptEngineData();
		s_Data->ScriptsFolder = config.ScriptsFolder;

		if (!FileSystem::Exists(config.ScriptsFolder))
		{
			ATN_CORE_ERROR_TAG("SriptEngine", "Scripts folder does not exists {}!", config.ScriptsFolder);
			return;
		}

		py::module_ sys = py::module_::import("sys");
		auto path = sys.attr("path");
		path.attr("insert")(0, s_Data->ScriptsFolder.string().c_str());
	}
	
	void PrivateScriptEngine::LoadScript(const String& name, Entity entity)
	{
		ATN_PROFILE_FUNC();

		if (s_Data->PythonModules.find(name) != s_Data->PythonModules.end())
		{
			ReloadScript(name, entity);
			return;
		}

		if (!ImportAndAddModule(name))
			return;

		s_Data->EntityClasses[name] = ScriptClass(name);

		const auto& fieldMapDesc = s_Data->EntityClasses.at(name).GetFieldsDescription();
		auto& fieldMap = s_Data->EntityScriptFields[entity.GetID()] = ScriptFieldMap();

		for (const auto& [name, field] : fieldMapDesc)
		{
			ScriptFieldStorage& storage = fieldMap[name];
			storage = field.Storage;
		}
	}

	void PrivateScriptEngine::ReloadScript(const String& name, Entity entity)
	{
		ATN_PROFILE_FUNC();

		bool exists = s_Data->EntityClasses.find(name) == s_Data->EntityClasses.end() ||
			s_Data->EntityScriptFields.find(entity.GetID()) == s_Data->EntityScriptFields.end();

		if (exists || !ReloadModule(name))
		{
			UnloadScript(name, entity);
			return;
		}

		s_Data->EntityClasses.at(name) = ScriptClass(name);

		const auto& fieldMapDesc = s_Data->EntityClasses.at(name).GetFieldsDescription();
		auto& fieldMap = s_Data->EntityScriptFields.at(entity.GetID());

		for (const auto& [name, field] : fieldMapDesc)
		{
			if (fieldMap.find(name) == fieldMap.end())
			{
				ScriptFieldStorage& storage = fieldMap[name];
				storage = field.Storage;
			}
		}

		ScriptFieldMap copy = fieldMap;
		for (const auto& [name, field] : copy)
		{
			if (fieldMapDesc.find(name) == fieldMapDesc.end())
				fieldMap.erase(name);
		}
	}

	void PrivateScriptEngine::UnloadScript(const String& name, Entity entity)
	{
		ATN_PROFILE_FUNC();

		UUID entityID = entity.GetID();

		s_Data->PythonModules.erase(name);
		s_Data->EntityClasses.erase(name);
		s_Data->EntityScriptFields.erase(entityID);
		s_Data->EntityInstances.erase(entityID);
	}

	bool PrivateScriptEngine::ExistsScript(const String& name)
	{
		return s_Data->PythonModules.find(name) != s_Data->PythonModules.end();
	}

	void PrivateScriptEngine::Shutdown()
	{
		delete s_Data;
	}

	void PrivateScriptEngine::OnRuntimeStart(Scene* scene)
	{
		s_Data->SceneContext = scene;
	}

	void PrivateScriptEngine::OnRuntimeStop()
	{
		s_Data->SceneContext = nullptr;
		s_Data->EntityInstances.clear();
	}

	ScriptFieldMap& PrivateScriptEngine::GetScriptFieldMap(Entity entity)
	{
		return s_Data->EntityScriptFields[entity.GetID()];
	}

	Scene* PrivateScriptEngine::GetSceneContext()
	{
		return s_Data->SceneContext;
	}

	bool PrivateScriptEngine::EntityClassExists(const String& name)
	{
		return s_Data->EntityClasses.find(name) != s_Data->EntityClasses.end();
	}

	bool PrivateScriptEngine::EntityInstanceExists(UUID uuid)
	{
		return s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end();
	}

	ScriptInstance& PrivateScriptEngine::GetScriptInstance(UUID uuid)
	{
		ATN_CORE_VERIFY(s_Data->EntityInstances.find(uuid) != s_Data->EntityInstances.end());
		return s_Data->EntityInstances.at(uuid);
	}

	const ScriptClass& PrivateScriptEngine::GetScriptClass(const String& name)
	{
		ATN_CORE_VERIFY(s_Data->EntityClasses.find(name) != s_Data->EntityClasses.end());
		return s_Data->EntityClasses.at(name);
	}

	void PrivateScriptEngine::InstantiateEntity(Entity entity)
	{
		ATN_PROFILE_FUNC();

		auto& sc = entity.GetComponent<ScriptComponent>();

		try
		{
			if (s_Data->EntityClasses.find(sc.Name) != s_Data->EntityClasses.end())
			{
				const ScriptClass& scriptClass = s_Data->EntityClasses.at(sc.Name);
				if(scriptClass.IsLoaded())
					s_Data->EntityInstances[entity.GetID()] = ScriptInstance(scriptClass, entity);
			}
		}
		catch (std::exception& exception)
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "InstantiateEntity({}) exception: \n{}!", entity.GetName(), exception.what());
		}
	}

	void PrivateScriptEngine::OnCreateEntity(Entity entity)
	{
		ATN_PROFILE_FUNC();

		auto& sc = entity.GetComponent<ScriptComponent>();

		try
		{
			if (s_Data->EntityClasses.find(sc.Name) != s_Data->EntityClasses.end())
				s_Data->EntityInstances[entity.GetID()].InvokeOnCreate();
		}
		catch (std::exception& exception)
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "OnCreateEntity({}) exception: \n{}!", entity.GetName(), exception.what());
		}
	}

	void PrivateScriptEngine::OnUpdateEntity(Entity entity, Time frameTime)
	{
		ATN_PROFILE_FUNC();

		auto& sc = entity.GetComponent<ScriptComponent>();

		try
		{
			if (s_Data->EntityClasses.find(sc.Name) != s_Data->EntityClasses.end())
				s_Data->EntityInstances[entity.GetID()].InvokeOnUpdate(frameTime);
		}
		catch (std::exception& exception)
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "OnUpdateEntity({}) exception: \n{}!", entity.GetName(), exception.what());
		}
	}

	std::vector<String> PrivateScriptEngine::GetAvailableModules()
	{
		std::vector<String> modules;

		for (const auto& dirEntry : std::filesystem::directory_iterator(s_Data->ScriptsFolder))
		{
			const auto& relativePath = dirEntry.path();
			const auto& filename = dirEntry.path().stem().string();

			modules.push_back(filename);
		}

		return modules;
	}
}
