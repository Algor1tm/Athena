#pragma once

#include "Core.h"
#include "Window.h"
#include "LayerStack.h"
#include "Events/ApplicationEvent.h"


namespace Athena
{
	class ATHENA_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void OnEvent(Event& event);

		void PushLayer(Layer* layer);
		void PopLayer(Layer* layer);
	private:
		bool OnWindowClose(const WindowCloseEvent& event);

		std::unique_ptr<Window> m_Window;
		bool m_Running;
	};

	// Defined by user
	Application* CreateApplication();
	LayerStack m_LayerStack;
}

