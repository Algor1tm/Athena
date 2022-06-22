#include "SandBox2D.h"

#include "ImGui/imgui.h"


SandBox2D::SandBox2D()
	: Layer("SandBox2D"), m_CameraController(16.f / 9.f, false), m_SquareColor(0.8f, 0.2f, 0.3f)
{

}

void SandBox2D::OnAttach()
{
	ATN_PROFILE_FUNCTION();

	m_CheckerBoard = Athena::Texture2D::Create("assets/textures/CheckerBoard.png");
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

	{
		ATN_PROFILE_SCOPE("Renderer Draw");
		Athena::Renderer2D::BeginScene(m_CameraController.GetCamera());
		Athena::Renderer2D::DrawRotatedQuad({ -1.f, 0.f }, { 0.8f, 0.8f }, 0.5f,  m_SquareColor);
		Athena::Renderer2D::DrawQuad({ 0.2f, -0.5f }, { 0.5f, 0.75f }, { 0.3f, 0.9f, 0.4f });
		Athena::Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.1f }, { 10.f, 10.f }, m_CheckerBoard, 10, Athena::Color::Green);
		Athena::Renderer2D::EndScene();
	}
}

void SandBox2D::OnImGuiRender()
{
	ATN_PROFILE_FUNCTION();

	ImGui::Begin("Settings");
	ImGui::ColorEdit4("Square Color", m_SquareColor.Data());
	ImGui::End();
}

void SandBox2D::OnEvent(Athena::Event& event)
{
	m_CameraController.OnEvent(event);
}
