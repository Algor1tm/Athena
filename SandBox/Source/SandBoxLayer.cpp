#include "SandBoxLayer.h"

using namespace Athena;


SandBoxLayer::SandBoxLayer(const FilePath& scenePath)
	: Layer("SandBox2D"), m_ScenePath(scenePath)
{

}

void SandBoxLayer::OnAttach()
{
	//m_Scene = CreateRef<Scene>();

	//SceneSerializer serializer(m_Scene);
	//if (serializer.DeserializeFromFile(m_ScenePath.string()))
	//{
	//	ATN_ERROR_TAG("SandBoxLayer", "Failed to load scene at '{}'", m_ScenePath);
	//}

	//m_Scene->OnRuntimeStart();

	//const auto& window = Application::Get().GetWindow();
	//m_Scene->OnViewportResize(window.GetWidth(), window.GetHeight());

	//auto& settings = SceneRenderer::GetSettings();

	//settings.BloomSettings.EnableBloom = false;
	//settings.ShadowSettings.FarPlaneOffset = 150.f;
	//settings.BloomSettings.Intensity = 1.2f;
}

void SandBoxLayer::OnDetach()
{

}

void SandBoxLayer::OnUpdate(Time frameTime)
{
	//SceneRenderer::BeginFrame();

	//m_Scene->OnUpdateRuntime(frameTime);

	//SceneRenderer::EndFrame();
	//SceneRenderer::GetFinalFramebuffer()->BlitToScreen();
}

bool SandBoxLayer::OnWindowResize(WindowResizeEvent& event)
{
	//m_Scene->OnViewportResize(event.GetWidth(), event.GetHeight());

	return false;
}

void SandBoxLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<WindowResizeEvent>(ATN_BIND_EVENT_FN(OnWindowResize));
}
