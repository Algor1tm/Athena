#include "Application.h"

#include "Athena/Core/Time.h"
#include "Athena/Core/Log.h"
#include "Athena/Core/FileSystem.h"


namespace Athena
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationCreateInfo& appinfo)
		: m_Running(true), m_Minimized(false)
	{
		ATN_CORE_VERIFY(s_Instance == nullptr, "Application already exists!");
		s_Instance = this;

		m_Config = appinfo.AppConfig;

		if (m_Config.WorkingDirectory != FilePath())
			FileSystem::SetWorkingDirectory(m_Config.WorkingDirectory);

		Log::Init(m_Config.EnableConsole);
		Renderer::Init(appinfo.RendererConfig);
		CreateMainWindow(appinfo.WindowInfo);
		Platform::Init();
		InitImGui();
		ScriptEngine::Init(appinfo.ScriptConfig);
	}

	Application::~Application()
	{
		ATN_PROFILER_SHUTDOWN()

		m_LayerStack.Clear();
		m_ImGuiLayer.Release();

		// Window cant be destroyed before Renderer::Shutdown, because of ImGui and GLFW
		m_Window->DestroySwapChain();
		Renderer::Shutdown();

		ScriptEngine::Shutdown();
		m_Window.Release();
	}

	void Application::Run()
	{
		Timer timer;
		Time frameTime = 0;

		// Initialize rendering stuff
		Renderer::WaitAndRender();

		while (m_Running)
		{
			ATN_PROFILE_FRAME("MainThread")

			Time start = timer.ElapsedTime();
			m_Statistics.FrameTime = frameTime;

			// Process Events
			ProcessEvents();

			if (m_Minimized == false)
			{
				// Wait for GPU commands to finish and begin new commands
				Renderer::BeginFrame();

				// Update
				{
					Timer timer = Timer();

					for (Ref<Layer> layer : m_LayerStack)
						layer->OnUpdate(frameTime);

					m_Statistics.Application_OnUpdate = timer.ElapsedTime();
				}

				// Render UI
				RenderImGui();

				// Submit commands to GPU and queue present
				Renderer::EndFrame();
				m_Window->SwapBuffers();
			}
			else
			{
				// Render ImGui viewports, that outside of window rect, when window minimized
				RenderImGui();

				// Immitate VSync when minimized
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}

			// Execute renderer commands
			Renderer::WaitAndRender();

			frameTime = timer.ElapsedTime() - start;
		}
	}

	void Application::ProcessEvents()
	{
		ATN_PROFILE_FUNC()
		Timer timer = Timer();

		m_Window->PollEvents();

		while (!m_EventQueue.empty())
		{
			Event& event = *m_EventQueue.front();
			OnEvent(event);
			m_EventQueue.pop();
		}

		m_Statistics.Application_ProcessEvents = timer.ElapsedTime();
	}

	void Application::RenderImGui()
	{
		Renderer::Submit([this]()
		{
			Timer timer = Timer();

			if (m_Config.EnableImGui)
			{
				ATN_PROFILE_SCOPE("Application::RenderImGui")

				m_ImGuiLayer->Begin();
				{
					for (Ref<Layer> layer : m_LayerStack)
						layer->OnImGuiRender();
				}
				m_ImGuiLayer->End(m_Minimized);
			}

			m_Statistics.Application_RenderImGui = timer.ElapsedTime();
		});
	}

	void Application::QueueEvent(const Ref<Event>& event)
	{
		m_EventQueue.push(event);
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(ATN_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(ATN_BIND_EVENT_FN(Application::OnWindowResized));

		for (LayerStack::iterator it = m_LayerStack.end(); it != m_LayerStack.begin();)
		{
			(*--it)->OnEvent(event);
			if (event.Handled)
				break;
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& event)
	{
		m_Running = false;
		return false;
	}

	bool Application::OnWindowResized(WindowResizeEvent& event)
	{
		if (event.GetWidth() == 0 || event.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Window->GetSwapChain()->OnWindowResize();

		m_Minimized = false;
		return false;
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::PushLayer(const Ref<Layer>& layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(const Ref<Layer>& layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::CreateMainWindow(WindowCreateInfo info)
	{
		if (info.EventCallback == nullptr)
			info.EventCallback = [this](const Ref<Event>& event) { Application::QueueEvent(event); };

		m_Window = Window::Create(info);
	}

	void Application::InitImGui()
	{
		if (m_Config.EnableImGui)
		{
			m_ImGuiLayer = ImGuiLayer::Create();
			PushOverlay(m_ImGuiLayer);
		}
		else
		{
			m_ImGuiLayer = nullptr;
		}
	}
}
