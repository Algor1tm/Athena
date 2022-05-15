#include "Application.h"
#include "Events/ApplicationEvent.h"
#include "Log.h"


namespace Athena
{
	Athena::Application::Application()
	{

	}


	Athena::Application::~Application()
	{

	}


	void Athena::Application::Run()
	{
		WindowResizeEvent event(1280, 720);
		ATN_FATAL("Created event");

		while (true);
	}
}
