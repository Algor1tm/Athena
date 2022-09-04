#pragma once

#include <Athena.h>


namespace Athena
{
	class SandBox2D : public Layer
	{
	public:
		SandBox2D();

		void OnAttach() override;
		void OnDetach() override;

		void OnUpdate(Time frameTime) override;
		void OnImGuiRender() override;
		void OnEvent(Event& event) override;

	private:
		Ref<Texture2D> m_SpriteSheet;
		Texture2DInstance m_Water, m_Dirt, m_Barrel;

		OrthographicCameraController m_CameraController;

		std::unordered_map<char, Texture2DInstance> m_TextureMap;
	};
}
