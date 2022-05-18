#include "atnpch.h"
#include "Application.h"
#include "Log.h"


namespace Athena
{
#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application()
		: m_Running(true)
	{
		ATN_CORE_ASSERT(s_Instance == nullptr, "Application already exists!");
		s_Instance = this;

		WindowDesc wdesc;

		m_Window = std::unique_ptr<Window>(Window::Create(wdesc));
		m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
	}


	Application::~Application()
	{

	}


	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

		ATN_CORE_INFO(event.ToString());

		for (LayerStack::iterator it = m_LayerStack.end(); it != m_LayerStack.begin();)
		{
			(*--it)->OnEvent(event);
			if (event.Handled)
				break;
		}
	}


	void Application::Run()
	{
		while (m_Running)
		{
			m_Window->OnUpdate();

			for (LayerStack::value_type layer: m_LayerStack)
				layer->OnUpdate();
		}
	}


	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
	}


	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
	}


	bool Application::OnWindowClose(const WindowCloseEvent& event)
	{
		m_Running = false;
		return true;
	}
}
