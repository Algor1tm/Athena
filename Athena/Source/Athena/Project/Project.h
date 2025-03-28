#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/UUID.h"
#include "Athena/Math/Vector.h"
#include "Athena/Math/Quaternion.h"
#include "Athena/Asset/RuntimeAssetManager.h"
#include "Athena/Asset/EditorAssetManager.h"


namespace Athena
{
	struct EditorState
	{
		UUID SelectedEntity = { 0 };
		FilePath ActiveScene;

		Vector3 CameraPos = Vector3(0);
		Vector2 CameraPitchYaw = Vector2(0, 0);
		float CameraSpeed = 0.3f;
	};

	struct ProjectConfig
	{
		// General
		String Name = "UnNamed";
		FilePath StartScene;

		// Scripting
		FilePath AthenaSourceDirectory;
		FilePath AthenaBinaryDirectory;

		// Physics
		uint32 VelocityIterations = 6;
		uint32 PositionIterations = 2;
		Vector2 Gravity = { 0, -9.8 };

		EditorState EditorSavedState;
	};

	class ATHENA_API Project
	{
	public:
		static const FilePath& GetProjectDirectory()
		{
			ATN_CORE_ASSERT(s_ActiveProject);
			return s_ActiveProject->m_ProjectDirectory;
		}

		static FilePath GetAssetDirectory()
		{
			return GetProjectDirectory() / s_AssetDirectory;
		}

		static FilePath GetAssetRegistryPath()
		{
			return GetAssetDirectory() / s_AssetRegistryPath;
		}

		static FilePath GetScriptsDirectory()
		{
			return GetProjectDirectory() / s_ScriptsDirectory;
		}

		static FilePath GetLogsDirectory()
		{
			return GetProjectDirectory() / s_LogsDirectory;
		}

		static FilePath GetScriptsBinaryPath()
		{
			return GetScriptsDirectory() / s_ScriptsBinaryPath;
		}

		FilePath GetProjectPath() const;
		ProjectConfig& GetConfig() { return m_Config; }

		static Ref<Project> GetActive() { return s_ActiveProject; }

		static Ref<AssetManagerBase> GetAssetManager() { return GetActive()->m_AssetManager; }
		static Ref<RuntimeAssetManager> GetRuntimeAssetManager() { return GetActive()->m_AssetManager.As<RuntimeAssetManager>(); }
		static Ref<EditorAssetManager> GetEditorAssetManager() { return GetActive()->m_AssetManager.As<EditorAssetManager>(); }

		static Ref<Project> New(const String& name, const FilePath& path);
		static Ref<Project> Load(const FilePath& path);
		static bool SaveActive(const FilePath& path);
		static bool SaveActive();
		static void Shutdown();

	private:
		static const inline FilePath s_AssetDirectory = "Assets";
		static const inline FilePath s_AssetRegistryPath = "AssetRegistry.athreg";
		static const inline FilePath s_ScriptsDirectory = "Scripts";
		static const inline FilePath s_LogsDirectory = "Logs";
		static const inline FilePath s_ScriptsBinaryPath = "Build/Binaries/ScriptsLibrary/ScriptsLibrary.dll";

	private:
		ProjectConfig m_Config;
		FilePath m_ProjectDirectory;
		Ref<AssetManagerBase> m_AssetManager;

		inline static Ref<Project> s_ActiveProject;
	};
}
