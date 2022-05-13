#pragma once

#include "Core.h"


namespace Athena
{
	class ATHENA_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	// Defined by user
	Application* CreateApplication();
}

