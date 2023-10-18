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

	uint32 width = 1280;
	uint32 height = 720;

	m_Camera->SetViewportSize(width, height);
	m_SceneRenderer->OnViewportResize(width, height);

	Application::Get().GetImGuiLayer()->BlockEvents(false);
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
}

void SandBoxLayer::OnImGuiRender()
{
	ImGui::Begin("Hello");

	Ref<Image> image = m_SceneRenderer->GetFinalImage();
	ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(), (float)image->GetHeight() });

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
	return false;
}

