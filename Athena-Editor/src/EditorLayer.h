#pragma once

#include <Athena.h>


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
		LinearColor m_SquareColor;
		Ref<Texture2D> m_CheckerBoard;
		Ref<Texture2D> m_KomodoHype;

		Vector2 m_ViewportSize = { 0, 0 };
		Ref<Framebuffer> m_Framebuffer;

		Ref<Scene> m_ActiveScene;
		Entity m_SquareEntity;

		bool m_ViewportFocused = true, m_ViewportHovered = true;
		OrthographicCameraController m_CameraController;
	};
}
