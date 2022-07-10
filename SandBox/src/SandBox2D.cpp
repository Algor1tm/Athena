#include "SandBox2D.h"

#include <ImGui/imgui.h>


SandBox2D::SandBox2D()
	: Layer("SandBox2D"), m_CameraController(16.f / 9.f, false), m_SquareColor(0.8f, 0.2f, 0.3f)
{

}

void SandBox2D::OnAttach()
{
	ATN_PROFILE_FUNCTION();

	m_CheckerBoard = Athena::Texture2D::Create("assets/textures/CheckerBoard.png");
	m_KomodoHype = Athena::Texture2D::Create("assets/textures/KomodoHype.png");
	m_SpriteSheet = Athena::Texture2D::Create("assets/game/textures/SpriteSheet.png");
	m_Stairs = Athena::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 7, 6 }, { 128, 128 });
	m_Tree = Athena::SubTexture2D::CreateFromCoords(m_SpriteSheet, { 2, 1 }, { 128, 128 }, {1, 2});
}

void SandBox2D::OnDetach()
{
	ATN_PROFILE_FUNCTION();
}

void SandBox2D::OnUpdate(Athena::Time frameTime)
{
	ATN_PROFILE_FUNCTION();

	m_CameraController.OnUpdate(frameTime);

	{
		ATN_PROFILE_SCOPE("Renderer Clear");
		Athena::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });
	}

	Athena::Renderer2D::ResetStats();
	{
#if 0
		static float rotation = 0.0f;
		rotation += frameTime.AsSeconds() * 1.f;

		ATN_PROFILE_SCOPE("Renderer Draw");
		Athena::Renderer2D::BeginScene(m_CameraController.GetCamera());

		Athena::Renderer2D::DrawQuad({ -1.f, 0.2f }, { 0.8f, 0.8f }, m_SquareColor);
		Athena::Renderer2D::DrawRotatedQuad({ 0.65f, 0.65f }, { 0.8f, 0.8f }, rotation, m_SquareColor);
		Athena::Renderer2D::DrawQuad({ 0.2f, -0.5f }, { 0.5f, 0.75f }, { 0.1f, 0.9f, 0.6f });
		Athena::Renderer2D::DrawQuad({ -0.f, -0.f, 0.1f }, { 10.f, 10.f }, m_CheckerBoard, 10, Athena::Color(1.f, 0.95f, 0.95f));
		Athena::Renderer2D::DrawRotatedQuad({ -0.7f, -0.7f }, { 1.f, 1.f }, Athena::DegreeToRad(45), m_KomodoHype);

		Athena::Renderer2D::EndScene();
#endif

		Athena::Renderer2D::BeginScene(m_CameraController.GetCamera());

		Athena::Renderer2D::DrawQuad({ 0.5f, 0, 0 }, { 1, 1 }, m_Stairs);
		Athena::Renderer2D::DrawQuad({ -0.5f, 0, 0 }, { 1, 2 }, m_Tree);

		Athena::Renderer2D::EndScene();
	}
}

void SandBox2D::OnImGuiRender()
{
	ATN_PROFILE_FUNCTION();

	ImGui::Begin("Settings");

	auto stats = Athena::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats: ");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCount);
	ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
	ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

	ImGui::ColorEdit4("Square Color", m_SquareColor.Data());

	ImGui::End();
}

void SandBox2D::OnEvent(Athena::Event& event)
{
	m_CameraController.OnEvent(event);
}
