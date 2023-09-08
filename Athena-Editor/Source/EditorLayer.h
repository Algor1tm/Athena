#pragma once

#include "Athena/Core/Layer.h"

#include "Athena/Input/KeyEvent.h"
#include "Athena/Input/MouseEvent.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/EditorCamera.h"

#include "Athena/Scene/Entity.h"

#include "PanelManager.h"
#include "ImGuizmoLayer.h"
#include "Titlebar.h"


namespace Athena
{
	class Scene;
	class Texture2D;

	class ViewportPanel;
	class MenuBarPanel;
	class SceneHierarchyPanel;
	class SettingsPanel;

	struct EditorConfig
	{
		FilePath EditorResources;
	};


	class EditorLayer : public Layer
	{
	public:
		EditorLayer(const EditorConfig& config);

		void OnAttach() override;
		void OnDetach() override;

		void OnUpdate(Time frameTime) override;
		void OnImGuiRender() override;
		void OnEvent(Event& event) override;

		Ref<Scene> GetActiveScene() { return m_ActiveScene; }

	private:
		Entity DuplicateEntity(Entity entity);

		void InitUI();
		void RenderOverlay();

		Entity GetEntityByCurrentMousePosition();

		bool OnKeyPressed(KeyPressedEvent& event);
		bool OnMouseReleased(MouseButtonReleasedEvent& event);

		void OnScenePlay();
		void OnSceneSimulate();
		void OnSceneStop();

		void NewScene();
		void SaveSceneAs();
		void SaveSceneAs(const FilePath& path);
		void OpenScene();
		void OpenScene(const FilePath& path);

	private:
		EditorConfig m_Config;

		Ref<Titlebar> m_Titlebar;
		bool m_HideCursor = false;

		Ref<EditorCamera> m_EditorCamera;

		ImGuizmoLayer m_ImGuizmoLayer;

		PanelManager m_PanelManager;
		Ref<ViewportPanel> m_MainViewport;
		Ref<SceneHierarchyPanel> m_SceneHierarchy;
		Ref<SettingsPanel> m_SettingsPanel;

		enum class SceneState
		{
			Edit = 0, Play = 1, Simulation = 2
		};

		SceneState m_SceneState = SceneState::Edit;
		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene, m_RuntimeScene;
		FilePath m_CurrentScenePath;
	};
}
