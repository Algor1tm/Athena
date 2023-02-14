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

	Application::Application(const ApplicationDescription& appdesc)
		: m_Running(true), m_Minimized(false)
	{
		ATN_CORE_ASSERT(s_Instance == nullptr, "Application already exists!");
		s_Instance = this;

		if (appdesc.WorkingDirectory != FilePath())
			FileSystem::SetWorkingDirectory(appdesc.WorkingDirectory);

		appdesc.UseConsole ? Log::Init() : Log::InitWithoutConsole();
			
		m_Window = Window::Create(appdesc.WindowDesc);
		Renderer::Init(appdesc.API);

		m_Window->SetEventCallback(ATN_BIND_EVENT_FN(Application::OnEvent));

		if (m_Window->GetWindowMode() != WindowMode::Default)
			Renderer::OnWindowResized(m_Window->GetWidth(), m_Window->GetHeight());

		if (appdesc.UseImGui)
		{
			m_ImGuiLayer = ImGuiLayer::Create();
			PushOverlay(m_ImGuiLayer);
		}
		else
		{
			m_ImGuiLayer = nullptr;
		}

		ScriptEngine::Init(appdesc.ScriptsFolder);
	}

	Application::~Application()
	{
		Renderer::Shutdown();
		ScriptEngine::Shutdown();
	}

	void Application::OnEvent(Event& event)
	{
		Timer timer;
		Time start = timer.ElapsedTime();

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(ATN_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(ATN_BIND_EVENT_FN(Application::OnWindowResized));

		for (LayerStack::iterator it = m_LayerStack.end(); it != m_LayerStack.begin();)
		{
			(*--it)->OnEvent(event);
			if (event.Handled)
				break;
		}

		m_Statistics.Application_OnEvent = timer.ElapsedTime() - start;
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
				{
					Time start = timer.ElapsedTime();

					for (Ref<Layer> layer : m_LayerStack)
					{
						layer->OnUpdate(frameTime);
					}

					m_Statistics.Application_OnUpdate = timer.ElapsedTime() - start;
				}

				if (m_ImGuiLayer != nullptr)
				{
					Time start = timer.ElapsedTime();

					m_ImGuiLayer->Begin();
					{
						for (Ref<Layer> layer : m_LayerStack)
							layer->OnImGuiRender();
					}
					m_ImGuiLayer->End();

					m_Statistics.Application_OnImGuiRender = timer.ElapsedTime() - start;
				}
			}

			{
				Time start = timer.ElapsedTime();
				m_Window->OnUpdate();
				m_Statistics.Window_OnUpdate = timer.ElapsedTime() - start;
			}

			frameTime = timer.ElapsedTime() - start;
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
		return true;
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
