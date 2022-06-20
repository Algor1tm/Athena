#include "SandBox2D.h"

#include "Athena/Platform/OpenGL/OpenGLShader.h"

#include "ImGui/imgui.h"


SandBox2D::SandBox2D()
	: Layer("SandBox2D"), m_CameraController(16.f / 9.f), m_SquareColor(0.8f, 0.2f, 0.3f)
{

}

void SandBox2D::OnAttach()
{

}

void SandBox2D::OnDetach()
{

}

void SandBox2D::OnUpdate(Athena::Time frameTime)
{
	m_CameraController.OnUpdate(frameTime);

	Athena::RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });

	Athena::Renderer2D::BeginScene(m_CameraController.GetCamera());
	Athena::Renderer2D::DrawQuad({ 0.f, 0.f }, { 1.f, 1.f }, m_SquareColor);
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
