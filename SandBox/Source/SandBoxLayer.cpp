#include "SandBoxLayer.h"

using namespace Athena;


SandBoxLayer::SandBoxLayer(const FilePath& scenePath)
	: Layer("SandBox2D"), m_ScenePath(scenePath)
{

}

void SandBoxLayer::OnAttach()
{
	m_SceneRenderer = SceneRenderer::Create();
	m_Scene = Ref<Scene>::Create();

	SceneSerializer serializer(m_Scene);
	if (serializer.DeserializeFromFile(m_ScenePath.string()))
	{
		ATN_INFO_TAG("SandBoxLayer", "Successfully load scene from {}", m_ScenePath);
	}
	else
	{
		ATN_ERROR_TAG("SandBoxLayer", "Failed to load scene from {}", m_ScenePath);
	}

	m_Scene->OnRuntimeStart();
}

void SandBoxLayer::OnDetach()
{
	m_SceneRenderer->Shutdown();
}

void SandBoxLayer::OnUpdate(Time frameTime)
{
	ATN_PROFILE_FUNC()

	m_Scene->OnUpdateRuntime(frameTime, m_SceneRenderer);
	Renderer::BlitToScreen(Renderer::GetRenderCommandBuffer(), m_SceneRenderer->GetFinalImage());
}

void SandBoxLayer::OnImGuiRender()
{
	
}

void SandBoxLayer::OnEvent(Event& event)
{
	ATN_PROFILE_FUNC()

	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<WindowResizeEvent>(ATN_BIND_EVENT_FN(OnWindowResize));
}

bool SandBoxLayer::OnWindowResize(WindowResizeEvent& event)
{
	ATN_PROFILE_FUNC()

	uint32 width = event.GetWidth();
	uint32 height = event.GetHeight();

	m_Scene->OnViewportResize(width, height);
	m_SceneRenderer->OnViewportResize(width, height);

	return false;
}

