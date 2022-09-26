#include "atnpch.h"
#include "Application.h"

#include "Log.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Input/Input.h"


namespace Athena
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationDescription& appdesc)
		: m_Running(true), m_Minimized(false)
	{
		ATN_CORE_ASSERT(s_Instance == nullptr, "Application already exists!");
		s_Instance = this;

		if (appdesc.WorkingDirectory != Filepath())
			std::filesystem::current_path(appdesc.WorkingDirectory);

		appdesc.UseConsole ? Log::Init() : Log::InitWithoutConsole();
			
		WindowDescription wdesc = appdesc.WindowDesc;

		RendererAPI::Init(appdesc.API);
		m_Window = Window::Create(wdesc);
		Renderer::Init();

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
	}

	Application::~Application()
	{
		Renderer::Shutdown();
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

	void Application::Run()
	{
		Timer Timer;
		Time LastTime;

		while (m_Running)
		{
			Time now = Timer.ElapsedTime();
			Time frameTime = now - LastTime;
			LastTime = now;

			if (m_Minimized == false)
			{
				{
					for (Ref<Layer> layer : m_LayerStack)
						layer->OnUpdate(frameTime);
				}

				if (m_ImGuiLayer != nullptr)
				{
					m_ImGuiLayer->Begin();
					{
						for (Ref<Layer> layer : m_LayerStack)
							layer->OnImGuiRender();
					}
					m_ImGuiLayer->End();
				}
			}

			m_Window->OnUpdate();
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
