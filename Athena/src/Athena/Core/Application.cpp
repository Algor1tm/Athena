#include "atnpch.h"
#include "Application.h"
#include "Log.h"

#include "Athena/Renderer/Renderer.h"

#include "Input.h"


namespace Athena
{
	Application* Application::s_Instance = nullptr;

	Application::Application()
		: m_Running(true), m_Minimized(false)
	{
		ATN_PROFILE_FUNCTION();

		ATN_CORE_ASSERT(s_Instance == nullptr, "Application already exists!");
		s_Instance = this;

		WindowDesc wdesc;

		m_Window = std::unique_ptr<Window>(Window::Create(wdesc));
		m_Window->SetEventCallback(ATN_BIND_EVENT_FN(Application::OnEvent));

		Renderer::Init();

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}


	Application::~Application()
	{
		ATN_PROFILE_FUNCTION();

		Renderer::Shutdown();
	}


	void Application::OnEvent(Event& event)
	{
		ATN_PROFILE_FUNCTION();

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(ATN_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizedEvent>(ATN_BIND_EVENT_FN(Application::OnWindowResized));

		for (LayerStack::iterator it = m_LayerStack.end(); it != m_LayerStack.begin();)
		{
			(*--it)->OnEvent(event);
			if (event.Handled)
				break;
		}
	}


	void Application::Run()
	{
		ATN_PROFILE_FUNCTION();

		Timer m_Timer;
		Time m_LastTime;

		while (m_Running)
		{
			ATN_PROFILE_SCOPE("Run Loop");

			Time now = m_Timer.ElapsedTime();
			Time frameTime = now - m_LastTime;
			m_LastTime = now;

			if (m_Minimized == false)
			{
				{
					ATN_PROFILE_SCOPE("LayerStack OnUpdate");

					for (Layer* layer : m_LayerStack)
						layer->OnUpdate(frameTime);
				}
			}

			m_ImGuiLayer->Begin();
			{
				ATN_PROFILE_FUNCTION("LayerStack OnImGuiRender");

				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();
			}
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
		}
	}


	void Application::PushLayer(Layer* layer)
	{
		ATN_PROFILE_FUNCTION();

		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}


	void Application::PushOverlay(Layer* layer)
	{
		ATN_PROFILE_FUNCTION();

		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}


	bool Application::OnWindowClose(WindowCloseEvent& event)
	{
		m_Running = false;
		return true;
	}


	bool Application::OnWindowResized(WindowResizedEvent& event)
	{
		ATN_PROFILE_FUNCTION();

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
