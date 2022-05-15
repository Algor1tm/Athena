#pragma once
 

#ifdef ATN_PLATFORM_WINDOWS

extern Athena::Application* Athena::CreateApplication();

int main(int argc, char** argv)
{
	Athena::Log::Init();
	ATN_CORE_WARN("Initialized core logger");
	ATN_INFO("Initialized client logger");

	Athena::Application* app = Athena::CreateApplication();
	app->Run();
	delete app;

	return 1;
}

#endif 
