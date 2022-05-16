#pragma once

#include "Core.h"
#include "Window.h"


namespace Athena
{
	class ATHENA_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running;
	};

	// Defined by user
	Application* CreateApplication();
}

