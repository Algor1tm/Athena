#pragma once

#include "Athena/Core/Layer.h"

#include "Athena/Input/KeyEvent.h"
#include "Athena/Input/MouseEvent.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/EditorCamera.h"

#include "Athena/Scene/Entity.h"

#include "PanelManager.h"
#include "EditorContext.h"
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

	private:
		Entity DuplicateEntity(Entity entity);

		void InitUI();
		void RenderOverlay();
		void DrawAboutModal();
		void DrawThemeEditor();

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

		Ref<EditorContext> m_EditorCtx;
		Ref<EditorCamera> m_EditorCamera;
		Ref<ImGuizmoLayer> m_ImGuizmoLayer;

		Ref<Titlebar> m_Titlebar;
		bool m_HideCursor = false;

		PanelManager m_PanelManager;
		Ref<ViewportPanel> m_MainViewport;
		Ref<SceneHierarchyPanel> m_SceneHierarchy;
		Ref<SettingsPanel> m_SettingsPanel;

		Ref<Scene> m_EditorScene, m_RuntimeScene;
		FilePath m_CurrentScenePath;

		bool m_AboutModalOpen = false;
		bool m_ThemeEditorOpen = false;
	};
}
