#include "SandBoxLayer.h"

using namespace Athena;


SandBoxLayer::SandBoxLayer(const FilePath& scenePath)
	: Layer("SandBox2D"), m_ScenePath(scenePath)
{

}

void SandBoxLayer::OnAttach()
{
	//m_SceneRenderer = SceneRenderer::Create();
	m_Camera = Ref<OrthographicCamera>::Create(-1.f, 1.f, -1.f, 1.f, true);

	//m_Scene = CreateRef<Scene>();

	//SceneSerializer serializer(m_Scene);
	//if (serializer.DeserializeFromFile(m_ScenePath.string()))
	//{
	//	ATN_ERROR_TAG("SandBoxLayer", "Failed to load scene at '{}'", m_ScenePath);
	//}

	//m_Scene->OnRuntimeStart();

	//auto& settings = SceneRenderer::GetSettings();

	//settings.BloomSettings.EnableBloom = false;
	//settings.ShadowSettings.FarPlaneOffset = 150.f;
	//settings.BloomSettings.Intensity = 1.2f;
}

void SandBoxLayer::OnDetach()
{
	//m_SceneRenderer->Shutdown();
}

void SandBoxLayer::OnUpdate(Time frameTime)
{
	Renderer::BeginFrame();

	// Temporary hack (scene renderer must be resized after Renderer::BeginFrame for now)
	if (m_ViewportResized)
	{
		//m_SceneRenderer->OnViewportResize(Application::Get().GetWindow().GetWidth(), Application::Get().GetWindow().GetHeight());
		m_ViewportResized = false;
	}

	m_Camera->OnUpdate(frameTime);

	CameraInfo cameraInfo;
	cameraInfo.ViewMatrix = m_Camera->GetViewMatrix();
	cameraInfo.ProjectionMatrix = m_Camera->GetProjectionMatrix();
	cameraInfo.NearClip = m_Camera->GetNearClip();
	cameraInfo.FarClip = m_Camera->GetFarClip();

	//m_SceneRenderer->Render(cameraInfo);

	Renderer::EndFrame();
}

void SandBoxLayer::OnImGuiRender()
{
	ImGui::Begin("Hello");
	ImGui::End();
}

void SandBoxLayer::OnEvent(Event& event)
{
	m_Camera->OnEvent(event);

	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<WindowResizeEvent>(ATN_BIND_EVENT_FN(OnWindowResize));
}

bool SandBoxLayer::OnWindowResize(WindowResizeEvent& event)
{
	m_Camera->SetViewportSize(event.GetWidth(), event.GetHeight());
	m_ViewportResized = true;

	return false;
}

