#include "SandBox2D.h"

#include <ImGui/imgui.h>


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
	ATN_PROFILE_FUNCTION();

	m_SpriteSheet = Athena::Texture2D::Create("assets/game/textures/SpriteSheet.png");
	m_TextureMap['D'] = Athena::SubTexture2D::CreateFromCoords(m_SpriteSheet, {6, 11}, {128, 128});
	m_TextureMap['W'] = Athena::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 11, 11 }, { 128, 128 });
	m_Barrel = Athena::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 9, 0 }, { 128, 128 });

	m_CameraController.SetZoomLevel(5.f);
}

void SandBox2D::OnDetach()
{
	ATN_PROFILE_FUNCTION();
}

void SandBox2D::OnUpdate(Athena::Time frameTime)
{
	ATN_PROFILE_FUNCTION();

	m_CameraController.OnUpdate(frameTime);

    Athena::Renderer2D::ResetStats();
	Athena::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });

	Athena::Renderer2D::BeginScene(m_CameraController.GetCamera());
		
	uint32_t mapHeight = (uint32_t)strlen(s_MapTiles) / s_MapWidth;
	for (uint32_t y = 0; y < mapHeight; ++y)
	{
		for (uint32_t x = 0; x < s_MapWidth; ++x)
		{
			char tileType = s_MapTiles[x + y * s_MapWidth];
			Athena::Ref<Athena::SubTexture2D> texture = m_Barrel;
			if (m_TextureMap.find(tileType) != m_TextureMap.end())
				texture = m_TextureMap[tileType];

			Athena::Renderer2D::DrawQuad({ (float)x - (float)s_MapWidth / 2.f, (float)y - (float)mapHeight / 2.f }, { 1, 1 }, texture);
		}
	}

	Athena::Renderer2D::EndScene();
}

void SandBox2D::OnImGuiRender()
{
	ATN_PROFILE_FUNCTION();

    ImGui::Begin("Renderer2D Stats");

    auto stats = Athena::Renderer2D::GetStats();
    ImGui::Text("Draw Calls: %d", stats.DrawCalls);
    ImGui::Text("Quads: %d", stats.QuadCount);
    ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
    ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

    ImGui::End();
}

void SandBox2D::OnEvent(Athena::Event& event)
{
	m_CameraController.OnEvent(event);
}
