#include "Application.h"

#include "Athena/Core/Time.h"
#include "Athena/Core/Log.h"
#include "Athena/Core/FileSystem.h"

#include "Athena/ImGui/ImGuiLayer.h"

#include "Athena/Renderer/Renderer.h"

#include "Athena/Input/Input.h"

#include "Athena/Scripting/ScriptEngine.h"


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
		InitImGui();
		ScriptEngine::Init(appinfo.ScriptConfig);
	}

	Application::~Application()
	{
		Renderer::WaitDeviceIdle();

		m_LayerStack.Clear();
		m_Window.Reset();
		ScriptEngine::Shutdown();
		Renderer::Shutdown();
	}

	void Application::Run()
	{
		Timer timer;
		Time frameTime = 0;

		while (m_Running)
		{
			Time start = timer.ElapsedTime();
			m_Statistics.FrameTime = frameTime;

			// Process Events
			ProcessEvents();

			if (m_Minimized == false)
			{
				Renderer::BeginFrame();
				// Update
				{
					Time start = timer.ElapsedTime();

					for (Ref<Layer> layer : m_LayerStack)
						layer->OnUpdate(frameTime);

					m_Statistics.Application_OnUpdate = timer.ElapsedTime() - start;
				}

				// Render UI
				UpdateImGui();

				Renderer::EndFrame();
				 
				// Swap Buffers
				{
					Time start = timer.ElapsedTime();
					m_Window->SwapBuffers();

					// Recreate SwapChain?
					if (m_Window->GetSwapChain()->Recreate())
					{
						if (m_ImGuiLayer)
							m_ImGuiLayer->OnSwapChainRecreate();
					}

					m_Statistics.Window_SwapBuffers = timer.ElapsedTime() - start;
				}
			}
			else
			{
				// Update ImGui viewports, that outside of window rect, when window minimized
				UpdateImGui();

				// Immitate VSync when minimized
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}

			frameTime = timer.ElapsedTime() - start;
		}
	}

	void Application::ProcessEvents()
	{
		//Time start = timer.ElapsedTime();

		m_Window->PollEvents();

		while (!m_EventQueue.empty())
		{
			Event& event = *m_EventQueue.front();
			OnEvent(event);
			m_EventQueue.pop();
		}

		//m_Statistics.Application_OnEvent = timer.ElapsedTime() - start;
	}

	void Application::UpdateImGui()
	{
		if (m_Config.EnableImGui)
		{
			//Time start = timer.ElapsedTime();

			m_ImGuiLayer->Begin();
			{
				for (Ref<Layer> layer : m_LayerStack)
					layer->OnImGuiRender();
			}
			m_ImGuiLayer->End(m_Minimized);

			//m_Statistics.Application_OnImGuiRender = timer.ElapsedTime() - start;
		}
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

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::PushLayer(Ref<Layer> layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Ref<Layer> layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
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

		m_Minimized = false;

		return false;
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
