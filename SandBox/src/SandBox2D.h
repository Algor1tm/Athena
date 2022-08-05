#pragma once

#include "Athena.h"


class SandBox2D: public Athena::Layer
{
public:
	SandBox2D();

	void OnAttach() override;
	void OnDetach() override;

	void OnUpdate(Athena::Time frameTime) override;
	void OnImGuiRender() override;
	void OnEvent(Athena::Event& event) override;

private:
	Athena::Color m_SquareColor;
	Athena::Ref<Athena::Texture2D> m_CheckerBoard;
	Athena::Ref<Athena::Texture2D> m_KomodoHype;
	Athena::Ref<Athena::Texture2D> m_SpriteSheet;
	Athena::Ref<Athena::SubTexture2D> m_Water, m_Dirt, m_Barrel;

	Athena::Ref<Athena::Framebuffer> m_Framebuffer;

	Athena::OrthographicCameraController m_CameraController;

	std::unordered_map<char, Athena::Ref<Athena::SubTexture2D>> m_TextureMap;
};
