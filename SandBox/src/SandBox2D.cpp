#include "SandBox2D.h"

#include "ImGui/imgui.h"


SandBox2D::SandBox2D()
	: Layer("SandBox2D"), m_CameraController(16.f / 9.f, false), m_SquareColor(0.8f, 0.2f, 0.3f)
{

}

void SandBox2D::OnAttach()
{
	m_CheckerBoard = Athena::Texture2D::Create("assets/textures/CheckerBoard.png");
}

void SandBox2D::OnDetach()
{

}

void SandBox2D::OnUpdate(Athena::Time frameTime)
{
	m_CameraController.OnUpdate(frameTime);

	Athena::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });

	Athena::Renderer2D::BeginScene(m_CameraController.GetCamera());
	Athena::Renderer2D::DrawQuad({ -1.f, 0.f }, { 0.8f, 0.8f }, m_SquareColor);
	Athena::Renderer2D::DrawQuad({ 0.2f, -0.5f }, { 0.5f, 0.75f }, { 0.3f, 0.9f, 0.4f });
	Athena::Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.1f }, { 10.f, 10.f }, m_CheckerBoard);
	Athena::Renderer2D::EndScene();
}

void SandBox2D::OnImGuiRender()
{
	ImGui::Begin("Settings");

	ImGui::ColorEdit4("Square Color", m_SquareColor.Data());

	ImGui::End();
}

void SandBox2D::OnEvent(Athena::Event& event)
{
	m_CameraController.OnEvent(event);
}
