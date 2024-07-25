#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Scene.h"
#include "Athena/Scripting/Script.h"
#include "Athena/Scripting/ScriptFields.h"


namespace Athena
{
	class ScriptClass
	{
	public:
		ScriptClass() = default;
		ScriptClass(const String& className);

		Script* Instantiate(Entity entity) const;

		ScriptFunc_OnCreate GetOnCreateMethod() { return m_OnCreateMethod; }
		ScriptFunc_OnUpdate GetOnUpdateMethod() { return m_OnUpdateMethod; }

		bool IsLoaded() const { return m_IsLoaded; }
		const String& GetName() const { return m_Name; }

		const ScriptFieldsDescription& GetFieldsDescription() const { return m_FieldsDescription; }

	private:
		ScriptFunc_Instantiate m_InstantiateMethod;
		ScriptFunc_OnCreate m_OnCreateMethod;
		ScriptFunc_OnUpdate m_OnUpdateMethod;

		ScriptFieldsDescription m_FieldsDescription;
		bool m_IsLoaded = false;
		String m_Name;
	};

	class ScriptInstance
	{
	public:
		ScriptInstance() = default;
		ScriptInstance(ScriptClass* scriptClass, Entity entity);
		~ScriptInstance();

		ScriptInstance(const ScriptInstance&) = delete;
		ScriptInstance& operator=(const ScriptInstance&) = delete;

		ScriptInstance(ScriptInstance&& other) = default;
		ScriptInstance& operator=(ScriptInstance&& other) = default;

		void InvokeOnCreate();
		void InvokeOnUpdate(Time frameTime);

		Script* GetInternalInstance();

	private:
		ScriptClass* m_ScriptClass;
		Script* m_Instance = nullptr;
	};


	struct ScriptConfig
	{
		FilePath ScriptsPath;
		FilePath ScriptsBinaryPath;
		bool EnableDebug;
	};

	class ATHENA_API ScriptEngine
	{
	public:
		static void Init(const ScriptConfig& config);
		static void Shutdown();

		static bool ReloadScripts();

		static bool IsScriptExists(const String& name);
		static std::vector<String> GetAvailableScripts();

		static void OnRuntimeStart(Scene* scene);
		static void OnRuntimeStop();

		static void InstantiateEntity(Entity entity);
		static void OnCreateEntity(Entity entity);
		static void OnUpdateEntity(Entity entity, Time frameTime);

	private:
		static void FindScripts(const FilePath& dir, std::vector<String>& scriptsNames);
	};
}
