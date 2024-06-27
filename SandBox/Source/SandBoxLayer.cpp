#include "SandBoxLayer.h"

using namespace Athena;


SandBoxLayer::SandBoxLayer(const FilePath& scenePath)
	: Layer("SandBox2D"), m_ScenePath(scenePath)
{

}

void SandBoxLayer::OnAttach()
{
	m_SceneRenderer = SceneRenderer::Create();

	m_Renderer2D = SceneRenderer2D::Create(m_SceneRenderer->GetRender2DPass());
	m_SceneRenderer->SetOnRender2DCallback(
		[this]() { OnRender2D(); });
	m_SceneRenderer->SetOnViewportResizeCallback(
		[this](uint32 width, uint32 height) { m_Renderer2D->OnViewportResize(width, height); });

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

	m_Scene->OnUpdateRuntime(frameTime);
	m_Scene->OnRender(m_SceneRenderer);

	Renderer::BlitToScreen(Renderer::GetRenderCommandBuffer(), m_SceneRenderer->GetFinalImage());

	if (Input::IsKeyPressed(Keyboard::Escape))
		Application::Get().Close();

	if (m_TimeToUpdate == 0.f)
	{
		UpdateStats();
	}
	else
	{
		m_TimeToUpdate -= frameTime.AsSeconds();
		m_TimeToUpdate = Math::Max(0.f, m_TimeToUpdate);
	}
}

void SandBoxLayer::OnEvent(Event& event)
{
	ATN_PROFILE_FUNC()

	EventDispatcher dispatcher(event);
	dispatcher.Dispatch<WindowResizeEvent>(ATN_BIND_EVENT_FN(OnWindowResize));
	dispatcher.Dispatch<KeyPressedEvent>(ATN_BIND_EVENT_FN(OnKeyPressedEvent));
}

bool SandBoxLayer::OnWindowResize(WindowResizeEvent& event)
{
	uint32 width = event.GetWidth();
	uint32 height = event.GetHeight();

	if (width != 0 && height != 0)
	{
		m_Scene->OnViewportResize(width, height);
		m_SceneRenderer->OnViewportResize(width, height);
	}

	return false;
}

bool SandBoxLayer::OnKeyPressedEvent(KeyPressedEvent& event)
{
	if (event.GetKeyCode() == Keyboard::F1)
	{
		m_ShowDefaultStats = !m_ShowDefaultStats;
	}
	else if (event.GetKeyCode() == Keyboard::F2)
	{
		m_ShowDetailedStats = !m_ShowDetailedStats;
	}
	else if (event.GetKeyCode() == Keyboard::F11)
	{
		auto& window = Application::Get().GetWindow();
		WindowMode currentMode = window.GetWindowMode();
		if (currentMode == WindowMode::Fullscreen)
		{
			window.SetWindowMode(WindowMode::Maximized);
		}
		else
		{
			window.SetWindowMode(WindowMode::Fullscreen);
		}
	}

	return false;
}

void SandBoxLayer::OnRender2D()
{
	Entity camera = m_Scene->GetPrimaryCameraEntity();
	auto& runtimeCamera = camera.GetComponent<CameraComponent>().Camera;
	Matrix4 view = Math::AffineInverse(camera.GetComponent<WorldTransformComponent>().AsMatrix());

	m_Renderer2D->BeginScene(view, runtimeCamera.GetProjectionMatrix());

	m_Scene->OnRender2D(m_Renderer2D);
	RenderUIOverlay();

	m_Renderer2D->EndScene();
}

void SandBoxLayer::UpdateStats()
{
	const auto& appstats = Application::Get().GetStats();

	uint32 fps = 1.f / appstats.FrameTime.AsSeconds();
	String RAM = Utils::MemoryBytesToString(Platform::GetMemoryUsage());
	String VRAM = Utils::MemoryBytesToString(Renderer::GetMemoryUsage());

	m_DefaultStats = fmt::format("FPS: {}\nRAM: {}\nVRAM: {}", fps, RAM, VRAM);

	float frameTime = appstats.FrameTime.AsMilliseconds();
	float gpuTime = m_SceneRenderer->GetStatistics().GPUTime.AsMilliseconds();
	float cpuWait = appstats.CPUWait.AsMilliseconds();
	uint32 entitiesCount = m_Scene->GetEntitiesCount();

	m_DetailedStats = fmt::format("FrameTime: {} ms\nGPUTime: {} ms\nCPUWait: {} ms\nEntities Count: {}", frameTime, gpuTime, cpuWait, entitiesCount);
	
	m_TimeToUpdate = 0.2f; // seconds
}

void SandBoxLayer::RenderUIOverlay()
{
	if (m_ShowDefaultStats)
		m_Renderer2D->DrawScreenSpaceText(m_DefaultStats, Font::GetDefault(), { 0.1f, 0.1f }, { 0.2f, 0.2f }, LinearColor::Green);

	if (m_ShowDetailedStats)
		m_Renderer2D->DrawScreenSpaceText(m_DetailedStats, Font::GetDefault(), { 0.1f, 8.f }, { 0.3f, 0.3f });
}
