#include "SandBox2D.h"

using namespace Athena;


SandBox2D::SandBox2D()
	: Layer("SandBox2D")
{

}

void SandBox2D::OnAttach()
{
	FilePath path = "Assets/Scenes/PBR_Example.atn";

	m_PBRScene = CreateRef<Scene>();

	SceneSerializer serializer(m_PBRScene);
	if (serializer.DeserializeFromFile(path.string()))
	{
		ATN_CORE_ERROR("Failed to load scene at '{}'", path);
	}

	m_PBRScene->OnRuntimeStart();

	const auto& window = Application::Get().GetWindow();
	m_PBRScene->OnViewportResize(window.GetWidth(), window.GetHeight());
}

void SandBox2D::OnDetach()
{

}

void SandBox2D::OnUpdate(Time frameTime)
{
	Renderer::BeginFrame();

	RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });

	m_PBRScene->OnUpdateRuntime(frameTime);

	Renderer::EndFrame();
	Renderer::BlitToScreen();
}

bool SandBox2D::OnWindowResize(WindowResizeEvent& event)
{
	m_PBRScene->OnViewportResize(event.GetWidth(), event.GetHeight());
	return true;
}

void SandBox2D::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<WindowResizeEvent>(ATN_BIND_EVENT_FN(SandBox2D::OnWindowResize));
}
