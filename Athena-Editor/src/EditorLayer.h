#pragma once

#include "Athena/Core/Layer.h"
#include "Athena/Core/Color.h"
#include "Athena/Core/OrthographicCameraController.h"
#include "Athena/Events/KeyEvent.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Framebuffer.h"
#include "Athena/Scene/Entity.h"

#include "Panels/SceneHierarchyPanel.h"


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
		bool OnKeyPressed(KeyPressedEvent& event);

		void NewScene();
		void SaveSceneAs();
		void OpenScene();

	private:
		Ref<Texture2D> m_CheckerBoard;
		Ref<Texture2D> m_KomodoHype;

		Vector2u m_ViewportSize = { 0, 0 };
		Ref<Framebuffer> m_Framebuffer;
		Time m_FrameTime;

		Ref<Scene> m_ActiveScene;
		Entity m_SquareEntity;
		Entity m_Komodo;
		Entity m_CameraEntity;

		bool m_ViewportFocused = true, m_ViewportHovered = true;
		OrthographicCameraController m_CameraController;

		SceneHierarchyPanel m_HierarchyPanel;
	};
}
