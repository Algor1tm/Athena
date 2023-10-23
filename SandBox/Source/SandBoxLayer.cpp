#include "SandBoxLayer.h"

using namespace Athena;


SandBoxLayer::SandBoxLayer(const FilePath& scenePath)
	: Layer("SandBox2D"), m_ScenePath(scenePath)
{

}

void SandBoxLayer::OnAttach()
{
	m_SceneRenderer = SceneRenderer::Create();
	m_Camera = Ref<OrthographicCamera>::Create(-1.f, 1.f, -1.f, 1.f, true);
}

void SandBoxLayer::OnDetach()
{
	m_SceneRenderer->Shutdown();
}

void SandBoxLayer::OnUpdate(Time frameTime)
{
	m_Camera->OnUpdate(frameTime);

	CameraInfo cameraInfo;
	cameraInfo.ViewMatrix = m_Camera->GetViewMatrix();
	cameraInfo.ProjectionMatrix = m_Camera->GetProjectionMatrix();
	cameraInfo.NearClip = m_Camera->GetNearClip();
	cameraInfo.FarClip = m_Camera->GetFarClip();

	m_SceneRenderer->Render(cameraInfo);

	Renderer::CopyTextureToSwapChain(m_SceneRenderer->GetFinalImage());
}

void SandBoxLayer::OnImGuiRender()
{

}

void SandBoxLayer::OnEvent(Event& event)
{
	m_Camera->OnEvent(event);

	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<WindowResizeEvent>(ATN_BIND_EVENT_FN(OnWindowResize));
}

bool SandBoxLayer::OnWindowResize(WindowResizeEvent& event)
{
	uint32 width = event.GetWidth();
	uint32 height = event.GetHeight();

	m_Camera->SetViewportSize(width, height);
	m_SceneRenderer->OnViewportResize(width, height);

	return false;
}

