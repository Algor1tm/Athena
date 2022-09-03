#pragma once
 

#ifdef ATN_PLATFORM_WINDOWS

extern Athena::Application* Athena::CreateApplication();

int main(int argc, char** argv)
{
	Athena::Log::Init();

	ATN_PROFILE_BEGIN_SESSION("Startup", "AthenaProfile-Startup.json");
	Athena::Application* app = Athena::CreateApplication();
	ATN_PROFILE_END_SESSION();

	ATN_PROFILE_BEGIN_SESSION("Startup", "AthenaProfile-Runtime.json");
	app->Run();
	ATN_PROFILE_END_SESSION();

	ATN_PROFILE_BEGIN_SESSION("Startup", "AthenaProfile-Shutdown.json");
	delete app;
	ATN_PROFILE_END_SESSION();

	return EXIT_SUCCESS;
}

#endif 
