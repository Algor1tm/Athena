#include "ScriptEngine.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/PlatformUtils.h"
#include "Athena/Scene/Entity.h"
#include "Athena/Scene/Components.h"
#include "Athena/Scripting/Script.h"
#include "Athena/Utils/StringUtils.h"


namespace Athena
{
	struct ScriptEngineData
	{
		ScriptConfig Config;
		Scope<Library> ScriptsLibrary;

		std::vector<String> ScriptsNames;
		std::unordered_map<String, ScriptClass> ScriptClasses;
		std::unordered_map<UUID, ScriptInstance> EntityInstances;
		std::unordered_map<UUID, ScriptFieldMap> EntityScriptFields;

		Scene* SceneContext = nullptr;
	};

	static ScriptEngineData* s_Data;

	template<typename FuncT, typename... Args>
	static bool InvokeScriptFunc(const String& scriptName, Entity entity, FuncT func, Args&&... args)
	{
		if (!s_Data->Config.EnableDebug)
		{
			func(std::forward<Args>(args)...);
			return true;
		}

		try
		{
			func(std::forward<Args>(args)...);
		}
		catch (std::exception& exception)
		{
			if (entity == Entity{})
			{
				ATN_CORE_ERROR_TAG("SriptEngine", "Script '{}' threw exception: \n{}!", scriptName, exception.what());
			}
			else
			{
				const String& entityName = entity.GetComponent<TagComponent>().Tag;
				UUID id = entity.GetComponent<IDComponent>().ID;

				ATN_CORE_ERROR_TAG("SriptEngine", "Script '{}' (Entity name - {}, id - {}) threw exception: \n{}!", 
					scriptName, entityName, id, exception.what());
			}
			return false;
		}

		return true;
	}

	ScriptClass::ScriptClass(const String& className)
	{
		m_InstantiateMethod = (ScriptFunc_Instantiate)s_Data->ScriptsLibrary->LoadFunction(fmt::format("_{}_Instantiate", className));
		m_OnCreateMethod = (ScriptFunc_OnCreate)s_Data->ScriptsLibrary->LoadFunction(fmt::format("_{}_OnCreate", className));
		m_OnUpdateMethod = (ScriptFunc_OnUpdate)s_Data->ScriptsLibrary->LoadFunction(fmt::format("_{}_OnUpdate", className));
		m_GetFieldsDescriptionMethod = (ScriptFunc_GetFieldsDescription)
			s_Data->ScriptsLibrary->LoadFunction(fmt::format("_{}_GetFieldsDescription", className));

		m_IsLoaded = m_InstantiateMethod && m_OnCreateMethod && m_OnUpdateMethod && m_GetFieldsDescriptionMethod;
		m_Name = className;

		if (!m_IsLoaded)
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to load script with name '{}'!", className);
			return;
		}

		// Get Fields
		Script* script = nullptr;
		if (InvokeScriptFunc(m_Name, Entity{}, m_InstantiateMethod, &script))
		{
			if (InvokeScriptFunc(m_Name, Entity{}, m_GetFieldsDescriptionMethod, script, &m_FieldsDescription))
			{
				for (auto& [name, storage] : m_FieldsDescription)
				{
					// Force to get initial value
					storage.GetValue<bool>();
					storage.SetInternalRef(nullptr);
				}

				delete script;
				return;
			}
		}

		ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to get fields description of script with name '{}'", className);
		delete script;
	}

	Script* ScriptClass::Instantiate(Entity entity) const
	{
		Script* script = nullptr;

		InvokeScriptFunc(m_Name, entity, m_InstantiateMethod, &script);
		script->Initialize(s_Data->SceneContext, entity);

		return script;
	}


	ScriptInstance::ScriptInstance(ScriptClass* scriptClass, Entity entity)
	{
		m_ScriptClass = scriptClass;
		m_Instance = m_ScriptClass->Instantiate(entity);

		if (s_Data->EntityScriptFields.contains(entity.GetID()))
		{
			ScriptFieldMap& entityFieldMap = s_Data->EntityScriptFields.at(entity.GetID());
			UpdateFieldMapRefs(entityFieldMap);
		}
	}

	ScriptInstance::~ScriptInstance()
	{
		delete m_Instance;
	}

	void ScriptInstance::UpdateFieldMapRefs(ScriptFieldMap& fieldMap)
	{
		ScriptFieldMap fieldRefs;
		InvokeScriptFunc(m_ScriptClass->GetName(), m_Instance->GetEntity(), m_ScriptClass->GetGetFieldsDescriptionMethod(),
			m_Instance, &fieldRefs);

		ATN_CORE_ASSERT(fieldMap.size() == fieldRefs.size());

		for (auto& [name, field] : fieldMap)
		{
			field.SetInternalRef(fieldRefs.at(name).GetInternalRef());
		}
	}

	void ScriptInstance::InvokeOnCreate()
	{
		InvokeScriptFunc(m_ScriptClass->GetName(), m_Instance->GetEntity(), m_ScriptClass->GetOnCreateMethod(),
			m_Instance);
	}

	void ScriptInstance::InvokeOnUpdate(Time frameTime)
	{
		InvokeScriptFunc(m_ScriptClass->GetName(), m_Instance->GetEntity(), m_ScriptClass->GetOnUpdateMethod(),
			m_Instance, (float)frameTime.AsMilliseconds());
	}

	Script* ScriptInstance::GetInternalInstance()
	{
		return m_Instance;
	}


	void ScriptEngine::Init(const ScriptConfig& config)
	{
		s_Data = new ScriptEngineData();
		s_Data->Config = config;

		if (!FileSystem::Exists(config.ScriptsPath))
		{
			ATN_CORE_ERROR_TAG("SriptEngine", "Scripts folder does not exists {}!", config.ScriptsPath);
			return;
		}

#ifdef ATN_DEBUG // This will search for scripting binary with MDd
		FilePath debugBinaryPath = s_Data->Config.ScriptsBinaryPath;
		debugBinaryPath = debugBinaryPath.parent_path() / FilePath(debugBinaryPath.stem().string() + "-d" + debugBinaryPath.extension().string());
		s_Data->Config.ScriptsBinaryPath = debugBinaryPath;
#endif

		GenerateCMakeConfig();
		ReloadScripts();
	}

	void ScriptEngine::Shutdown()
	{
		delete s_Data;
	}

	bool ScriptEngine::ReloadScripts()
	{
		s_Data->ScriptsLibrary.Release();
		s_Data->ScriptClasses.clear();
		s_Data->ScriptsNames.clear();
		s_Data->EntityScriptFields.clear();

		if (!FileSystem::Exists(s_Data->Config.ScriptsBinaryPath))
		{
			ATN_CORE_ERROR_TAG("SriptEngine", "Scripts binary does not exists {}!", s_Data->Config.ScriptsBinaryPath);
			return false;
		}

		// Create copy of dll and pdb to read from it, another dll for writing
		const FilePath& libPath = s_Data->Config.ScriptsBinaryPath;
		FilePath pdbPath = s_Data->Config.ScriptsBinaryPath;
		pdbPath.replace_extension("pdb");

		FilePath activeFolder = libPath.parent_path() / "Active";

		if (!FileSystem::Exists(activeFolder))
		{
			FileSystem::CreateDirectory(activeFolder);
		}

		FilePath activeLibPath = activeFolder / libPath.filename();
		FilePath activePdbPath = activeFolder / pdbPath.filename();

		if (!FileSystem::Copy(libPath, activeLibPath))
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to load scripting library!");
			return false;
		}

		if (FileSystem::Exists(pdbPath))
		{
			if (!FileSystem::Copy(pdbPath, activePdbPath))
			{
				ATN_CORE_WARN_TAG("ScriptEngine", "Failed to load debug info!");
			}
		}
		
		FilePath sourceDir = s_Data->Config.ScriptsPath / "Source";

		if (!FileSystem::Exists(sourceDir))
		{
			ATN_CORE_ERROR_TAG("SriptEngine", "Source directory does not exists {}!", sourceDir);
			return false;
		}

		s_Data->ScriptsLibrary = Scope<Library>::Create(activeLibPath);
		if (!s_Data->ScriptsLibrary->IsLoaded())
			return false;

		FindScripts(sourceDir, s_Data->ScriptsNames);

		for (const auto& scriptName : s_Data->ScriptsNames)
		{
			ScriptClass scriptClass = ScriptClass(scriptName);

			if (scriptClass.IsLoaded())
				s_Data->ScriptClasses[scriptName] = scriptClass;
		}

		if (s_Data->ScriptsNames.empty())
			ATN_CORE_WARN_TAG("ScriptEngine", "Failed to find any scripts in scripts binary!");

		return true;
	}

	bool ScriptEngine::IsScriptExists(const String& name)
	{
		return std::find(s_Data->ScriptsNames.begin(), s_Data->ScriptsNames.end(), name) != s_Data->ScriptsNames.end();
	}

	const std::vector<String>& ScriptEngine::GetAvailableScripts()
	{
		return s_Data->ScriptsNames;
	}

	ScriptFieldMap* ScriptEngine::GetScriptFieldMap(Entity entity)
	{
		if (s_Data->EntityScriptFields.contains(entity.GetID()))
			return &s_Data->EntityScriptFields.at(entity.GetID());

		const auto& scriptName = entity.GetComponent<ScriptComponent>().Name;

		if (s_Data->ScriptClasses.contains(scriptName))
		{
			ScriptClass scClass = s_Data->ScriptClasses.at(scriptName);
			uint32 fieldsCount = scClass.GetFieldsDescription().size();
			if (fieldsCount != 0)
			{
				ScriptFieldMap& fieldMap = s_Data->EntityScriptFields[entity.GetID()] = scClass.GetFieldsDescription();

				// Update refs if initialize fieldMap at runtime
				if (s_Data->EntityInstances.contains(entity.GetID()))
				{
					s_Data->EntityInstances.at(entity.GetID()).UpdateFieldMapRefs(fieldMap);
				}

				return &fieldMap;
			}
		}

		return nullptr;
	}

	void ScriptEngine::ClearEntityFieldMap(Entity entity)
	{
		if(s_Data->EntityScriptFields.contains(entity.GetID()))
			s_Data->EntityScriptFields.erase(entity.GetID());
	}

	void ScriptEngine::OnRuntimeStart(Scene* scene)
	{
		s_Data->SceneContext = scene;
	}

	void ScriptEngine::OnRuntimeStop()
	{
		for (auto& [entity, fieldMap] : s_Data->EntityScriptFields)
		{
			for (auto& [name, field] : fieldMap)
			{
				field.SetInternalRef(nullptr);
			}
		}

		s_Data->SceneContext = nullptr;
		s_Data->EntityInstances.clear();
	}

	void ScriptEngine::InstantiateEntity(Entity entity)
	{
		ATN_PROFILE_FUNC();

		const String& scriptName = entity.GetComponent<ScriptComponent>().Name;

		if (s_Data->ScriptClasses.contains(scriptName))
		{
			ScriptClass& scriptClass = s_Data->ScriptClasses.at(scriptName);

			// Need to construct ScriptInstance in place
			s_Data->EntityInstances.emplace(std::piecewise_construct, 
				std::forward_as_tuple(entity.GetID()), 
				std::forward_as_tuple(&scriptClass, entity));
		}
	}

	void ScriptEngine::OnCreateEntity(Entity entity)
	{
		ATN_PROFILE_FUNC();

		const String& scriptName = entity.GetComponent<ScriptComponent>().Name;

		if (s_Data->ScriptClasses.contains(scriptName))
		{
			s_Data->EntityInstances.at(entity.GetID()).InvokeOnCreate();
		}
	}

	void ScriptEngine::OnUpdateEntity(Entity entity, Time frameTime)
	{
		ATN_PROFILE_FUNC();

		const String& scriptName = entity.GetComponent<ScriptComponent>().Name;

		if (s_Data->ScriptClasses.contains(scriptName))
		{
			s_Data->EntityInstances.at(entity.GetID()).InvokeOnUpdate(frameTime);
		}
	}

	void ScriptEngine::GenerateCMakeConfig()
	{
		FilePath scResources = Application::Get().GetConfig().EngineResourcesPath / "Scripting";
		FilePath configTemplate = scResources / "Config-Template.cmake";

		if (!FileSystem::Exists(configTemplate))
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to find config template!");
			return;
		}

		// TODO: Projects
		String configSource = FileSystem::ReadFile(configTemplate);
		Utils::ReplaceAll(configSource, "<REPLACE_PROJECT_NAME>", "Sandbox");
		Utils::ReplaceAll(configSource, "<REPLACE_ATHENA_SOURCE_DIR>", "${CMAKE_SOURCE_DIR}/../../..");
		Utils::ReplaceAll(configSource, "<REPLACE_ATHENA_BINARY_DIR>", "${CMAKE_SOURCE_DIR}/../../../Build/Binaries/Athena");
		Utils::ReplaceAll(configSource, "<REPLACE_USE_DEBUG_RUNTIME_LIBS>", "ON");

		FilePath configPath = s_Data->Config.ScriptsPath / "Config.cmake";
		if (!FileSystem::WriteFile(configPath, configSource.c_str(), configSource.size()))
			ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to generate cmake config {}", configPath);
	}

	void ScriptEngine::CreateNewScript(const String& name)
	{
		bool invalidName = name.empty() ||
			std::find_if(name.begin(), name.end(), [](char c) { return !std::isalpha(c); }) != name.end();

		if (invalidName)
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "Invalid name for script class - '{}'", name);
			return;
		}

		FilePath scResources = Application::Get().GetConfig().EngineResourcesPath / "Scripting";

		FilePath cppTemplate = scResources / "Script-Template.cpp";
		FilePath headerTemplate = scResources / "Script-Template.h";

		if (!FileSystem::Exists(cppTemplate) || !FileSystem::Exists(headerTemplate))
		{
			ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to find script templates!");
			return;
		}

		FilePath srcPath = s_Data->Config.ScriptsPath / "Source";

		{
			String headerSource = FileSystem::ReadFile(headerTemplate);
			Utils::ReplaceAll(headerSource, "ClassName", name);
			Utils::ReplaceAll(headerSource, "NamespaceName", "Sandbox");	// TODO: Projects
			FilePath headerPath = srcPath / fmt::format("{}.h", name);

			if (!FileSystem::WriteFile(headerPath, headerSource.c_str(), headerSource.size()))
				ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to create new script file {}", headerPath);
		}

		{
			String cppSource = FileSystem::ReadFile(cppTemplate);
			Utils::ReplaceAll(cppSource, "ClassName", name);
			Utils::ReplaceAll(cppSource, "NamespaceName", "Sandbox");	// TODO: Projects
			FilePath cppPath = srcPath / fmt::format("{}.cpp", name);

			if(!FileSystem::WriteFile(cppPath, cppSource.c_str(), cppSource.size()))
				ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to create new script file {}", cppPath);
		}

		GenCMakeProjects();
	}

	void ScriptEngine::OpenInVisualStudio()
	{
		const String projectName = "Sandbox"; // Temporary

		FilePath solutionName = fmt::format("{}.sln", projectName);
		FilePath solutionPath = s_Data->Config.ScriptsPath / "Build/Projects" / solutionName;

		if (!FileSystem::Exists(solutionPath))
			GenCMakeProjects();

		if (FileSystem::Exists(solutionPath))
			Platform::RunFile(solutionName, s_Data->Config.ScriptsPath / "Build/Projects");
		else
			ATN_CORE_ERROR_TAG("ScriptEngine", "Failed to open solution file!");
	}

	void ScriptEngine::GenCMakeProjects()
	{
		Platform::RunFile("VS2022-GenProjects.bat", s_Data->Config.ScriptsPath);
	}

	void ScriptEngine::FindScripts(const FilePath& dir, std::vector<String>& scriptsNames)
	{
		// TODO: in the future at runtime we will be using AssetManager to find necessary scripts
		// For now iterate through .cpp files and check if corresponding script exists 
		// in library and then load this script

		for (const auto& dirEntry : std::filesystem::directory_iterator(dir))
		{
			const FilePath& path = dirEntry.path();

			if (dirEntry.is_directory())
				FindScripts(path, scriptsNames);

			if (path.extension() == ".cpp")
			{
				String name = path.stem().string();

				String instantiateFuncName = fmt::format("_{}_Instantiate", name);
				void* func = s_Data->ScriptsLibrary->LoadFunction(instantiateFuncName);

				if (func)
					scriptsNames.push_back(name);
			}
		}
	}
}
