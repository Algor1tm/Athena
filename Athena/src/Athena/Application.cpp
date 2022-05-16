#include "atnpch.h"
#include "Application.h"
#include "Events/ApplicationEvent.h"
#include "Log.h"


namespace Athena
{
	Athena::Application::Application()
		: m_Running(true)
	{
		WindowDesc wdesc;
		wdesc.Width = 1280;
		wdesc.Height = 720;
		wdesc.Title = "Athena Engine";
		wdesc.VSync = true;

		m_Window = std::unique_ptr<Window>(Window::Create(wdesc));
	}


	Athena::Application::~Application()
	{

	}


	void Athena::Application::Run()
	{
		while (m_Running)
		{
			m_Window->OnUpdate();
		}
	}
}
