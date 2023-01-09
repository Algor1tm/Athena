#include "SandBox2D.h"



namespace Athena
{
	static const uint32_t s_MapWidth = 24;
	static const char s_MapTiles[] =
		"WWWWWWWWWWWWWWWWWWWWWWWW"
		"WWWWWWWWWWWWWWWWWWWWWWWW"
		"WWWWWWWWWDDDDDWWWWWWWWWW"
		"WWWWWDDDDDDDDDDDDDWWWWWW"
		"WWWWDDDDDDDWWDDDDDDDWWWW"
		"WWDDDDDDDDDDDDDDDDDDDWWW"
		"WWDDDDDDDDDDDDDDDDDDDWWW"
		"WWDDDDDDDDDDDDDDDDDDDWWW"
		"WWDDDDDDDDDDDDDDDDDDDWWW"
		"WWWDDDDWDDDDDDDDDDDDWWWW"
		"WWWWWDDDDDDDDDDDDDWWWWWW"
		"WWWWWWWWWDDDDDWWWWWWWWWW"
		"WWWWWWWWWWWWWWWWWWWWWWWW";


	SandBox2D::SandBox2D()
		: Layer("SandBox2D"), m_CameraController(16.f / 9.f, false)
	{

	}

	void SandBox2D::OnAttach()
	{
		m_SpriteSheet = Texture2D::Create("Assets/Textures/SpriteSheet.png");
		m_TextureMap['D'] = Texture2DInstance(m_SpriteSheet, { 6 * 128, 11 * 128 }, { 7 * 128, 12 * 128 });
		m_TextureMap['W'] = Texture2DInstance(m_SpriteSheet, { 11 * 128, 11 * 128 }, { 12 * 128, 12 * 128 });
		m_Barrel = Texture2DInstance(m_SpriteSheet, { 9 * 128, 0 }, { 9 * 128, 1 * 128 });

		m_CameraController.SetZoomLevel(5.f);
	}

	void SandBox2D::OnDetach()
	{

	}

	void SandBox2D::OnUpdate(Time frameTime)
	{
		m_CameraController.OnUpdate(frameTime);

		Renderer2D::ResetStats();
		RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });

		Renderer2D::BeginScene(m_CameraController.GetCamera().GetViewProjectionMatrix());

		uint32_t mapHeight = (uint32_t)strlen(s_MapTiles) / s_MapWidth;
		for (uint32_t y = 0; y < mapHeight; ++y)
		{
			for (uint32_t x = 0; x < s_MapWidth; ++x)
			{
				char tileType = s_MapTiles[x + y * s_MapWidth];
				Texture2DInstance texture = m_Barrel;
				if (m_TextureMap.find(tileType) != m_TextureMap.end())
					texture = m_TextureMap[tileType];

				Renderer2D::DrawQuad({ (float)x - (float)s_MapWidth / 2.f, (float)y - (float)mapHeight / 2.f }, { 1, 1 }, texture);
			}
		}

		Renderer2D::EndScene();
	}

	void SandBox2D::OnImGuiRender()
	{

	}

	void SandBox2D::OnEvent(Event& event)
	{
		m_CameraController.OnEvent(event);
	}
}
