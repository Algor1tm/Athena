#pragma once

#include "Athena/Core/Layer.h"

#include "Athena/Input/Events/KeyEvent.h"
#include "Athena/Input/Events/MouseEvent.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Editorcamera.h"

#include "Athena/Scene/Entity.h"

#include "PanelManager.h"

#include "ImGuizmoLayer.h"


namespace Athena
{
	class Scene;
	class Texture2D;

	class ViewportPanel;
	class MenuBarPanel;
	class SceneHierarchyPanel;
	class EditorSettingsPanel;


	class EditorLayer : public Layer
	{
	public:
		EditorLayer();

		void OnAttach() override;
		void OnDetach() override;

		void OnUpdate(Time frameTime) override;
		void OnImGuiRender() override;
		void OnEvent(Event& event) override;

	private:
		void SelectEntity(Entity entity);
		void InitializePanels();
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
		EditorCamera m_EditorCamera;
		Entity m_SelectedEntity = {};

		ImGuizmoLayer m_ImGuizmoLayer;

		PanelManager m_PanelManager;
		Ref<ViewportPanel> m_MainViewport;
		Ref<MenuBarPanel> m_MainMenuBar;
		Ref<SceneHierarchyPanel> m_SceneHierarchy;
		Ref<EditorSettingsPanel> m_EditorSettings;

		enum class SceneState
		{
			Edit = 0, Play = 1, Simulation = 2
		};

		SceneState m_SceneState = SceneState::Edit;
		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene, m_RuntimeScene;
		FilePath m_TemporaryEditorScenePath;
		FilePath m_CurrentScenePath;

		Ref<Texture2D> m_PlayIcon;
		Ref<Texture2D> m_StopIcon;
		Ref<Texture2D> m_SimulationIcon;
	};
}
