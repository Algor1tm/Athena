#include "atnpch.h"
#include "Application.h"
#include "Log.h"

#include "Athena/Renderer/Renderer.h"

#include "Input.h"


namespace Athena
{
	Application* Application::s_Instance = nullptr;

	Application::Application()
		: m_Running(true)
	{
		ATN_CORE_ASSERT(s_Instance == nullptr, "Application already exists!");
		s_Instance = this;

		WindowDesc wdesc;
		wdesc.Width = 800;
		wdesc.Height = 600;
		wdesc.VSync = false;

		m_Window = std::unique_ptr<Window>(Window::Create(wdesc));
		m_Window->SetEventCallback(ATN_BIND_EVENT_FN(Application::OnEvent));

		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}


	Application::~Application()
	{

	}


	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(ATN_BIND_EVENT_FN(Application::OnWindowClose));

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
			Time now = m_Timer.GetElapsedTime();
			Time frameTime = now - m_LastTime;
			m_LastTime = now;

			for (Layer* layer: m_LayerStack)
				layer->OnUpdate(frameTime);

			m_ImGuiLayer->Begin();
			for (Layer* layer: m_LayerStack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
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


	bool Application::OnWindowClose(WindowCloseEvent& event)
	{
		m_Running = false;
		return true;
	}
}
