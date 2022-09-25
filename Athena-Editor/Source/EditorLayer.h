#pragma once

#include "Athena/Core/Layer.h"
#include "Athena/Core/Color.h"

#include "Athena/Input/Events/KeyEvent.h"
#include "Athena/Input/Events/MouseEvent.h"

#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Framebuffer.h"
#include "Athena/Renderer/Editorcamera.h"

#include "Athena/Scene/Entity.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"

#include <ImGuizmo/ImGuizmo.h>


namespace Athena
{
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
		void Toolbar();
		void RenderOverlay();

		Entity GetEntityByCurrentMousePosition();

		bool OnKeyPressed(KeyPressedEvent& event);
		bool OnMouseReleased(MouseButtonReleasedEvent& event);

		void OnScenePlay();
		void OnSceneSimulate();
		void OnSceneStop();

		void NewScene();
		void SaveSceneAs();
		void SaveSceneAs(const Filepath& path);
		void OpenScene();
		void OpenScene(const Filepath& path);

	private:
		bool m_ViewportFocused = true, m_ViewportHovered = true;
		Vector2u m_ViewportSize = { 0, 0 };
		Vector2 m_ViewportBounds[2] = {};
		Ref<Framebuffer> m_Framebuffer;

		Ref<Scene> m_ActiveScene;
		Ref<Scene> m_EditorScene, m_RuntimeScene;
		Filepath m_TemporaryEditorScenePath;
		Filepath m_CurrentScenePath;
		EditorCamera m_EditorCamera;
		Entity m_SelectedEntity = {};
		ImGuizmo::OPERATION m_GuizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
		bool m_ShowColliders = false;

		SceneHierarchyPanel m_HierarchyPanel;
		ContentBrowserPanel m_ContentBrowserPanel;
		bool m_ContentBrowserRendering = true;
		Time m_FrameTime;

		enum class SceneState
		{
			Edit = 0, Play = 1, Simulation = 2
		};

		SceneState m_SceneState = SceneState::Edit;
		Ref<Texture2D> m_PlayIcon;
		Ref<Texture2D> m_StopIcon;
		Ref<Texture2D> m_SimulationIcon;
	};
}
