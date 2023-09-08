#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Scene.h"
#include "Athena/Scene/Entity.h"


//namespace Athena
//{
//	enum class SceneState
//	{
//		Edit = 0, Play = 1, Simulation = 2
//	};
//
//
//	class SceneManager
//	{
//	public:
//
//		void NewScene();
//		void SaveSceneAs();
//		void SaveSceneAs(const FilePath& path);
//		void OpenScene();
//		void OpenScene(const FilePath& path);
//
//
//	private:
//		SceneState m_SceneState = SceneState::Edit;
//		Ref<Scene> m_ActiveScene;
//		Ref<Scene> m_EditorScene, m_RuntimeScene;
//		FilePath m_CurrentScenePath;
//
//		Entity m_SelectedEntity = {};
//	};
//}
