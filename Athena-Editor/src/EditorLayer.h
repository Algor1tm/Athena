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
		Color m_SquareColor;
		Ref<Texture2D> m_CheckerBoard;
		Ref<Texture2D> m_KomodoHype;
		Ref<Texture2D> m_SpriteSheet;
		Ref<SubTexture2D> m_Water, m_Dirt, m_Barrel;

		Vector2 m_ViewportSize = { 0, 0 };
		Ref<Framebuffer> m_Framebuffer;

		OrthographicCameraController m_CameraController;

		std::unordered_map<char, Ref<SubTexture2D>> m_TextureMap;
	};
}
