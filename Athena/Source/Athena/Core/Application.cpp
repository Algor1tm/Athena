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

		m_Name = appinfo.AppConfig.Name;
		m_EngineResourcesPath = appinfo.AppConfig.EngineResources;

		if (appinfo.AppConfig.WorkingDirectory != FilePath())
			FileSystem::SetWorkingDirectory(appinfo.AppConfig.WorkingDirectory);

		Log::Init(appinfo.AppConfig.EnableConsole);
			
		Renderer::Init(appinfo.RendererConfig);

		m_Window = Window::Create(appinfo.WindowInfo);
		m_Window->SetEventCallback([this](const Ref<Event>& event) { Application::QueueEvent(event); });
		QueueEvent(Ref<WindowResizeEvent>::Create(m_Window->GetWidth(), m_Window->GetHeight()));

		ScriptEngine::Init(appinfo.ScriptConfig);

		if (appinfo.AppConfig.EnableImGui)
		{
			m_ImGuiLayer = ImGuiLayer::Create();
			PushOverlay(m_ImGuiLayer);
		}
		else
		{
			m_ImGuiLayer = nullptr;
		}
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

			if (m_Minimized == false)
			{
				// Process Event Queue
				{
					Time start = timer.ElapsedTime();

					while (!m_EventQueue.empty())
					{
						Event& event = *m_EventQueue.front();
						OnEvent(event);
						m_EventQueue.pop();
					}

					m_Statistics.Application_OnEvent = timer.ElapsedTime() - start;
				}

				// Update
				{
					Time start = timer.ElapsedTime();

					for (Ref<Layer> layer : m_LayerStack)
					{
						layer->OnUpdate(frameTime);
					}

					m_Statistics.Application_OnUpdate = timer.ElapsedTime() - start;
				}

				// Render UI
				if (m_ImGuiLayer != nullptr)
				{
					Time start = timer.ElapsedTime();

					m_ImGuiLayer->Begin();
					{
						for (Ref<Layer> layer : m_LayerStack)
						{
							layer->OnImGuiRender();
						}
					}
					m_ImGuiLayer->End();

					m_Statistics.Application_OnImGuiRender = timer.ElapsedTime() - start;
				}
			}

			// Swap Buffers
			{
				Time start = timer.ElapsedTime();
				m_Window->OnUpdate();

				// Recreate SwapChain?
				if (m_Window->GetSwapChain()->Recreate())
				{
					m_ImGuiLayer->OnSwapChainRecreate();
				}

				m_Statistics.Window_OnUpdate = timer.ElapsedTime() - start;
			}

			frameTime = timer.ElapsedTime() - start;
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

		Renderer::OnWindowResized(event.GetWidth(), event.GetHeight());

		return false;
	}
}
