#pragma once
 

#ifdef ATN_PLATFORM_WINDOWS

extern Athena::Application* Athena::CreateApplication();

int main(int argc, char** argv)
{
	Athena::Application* app = Athena::CreateApplication();
	app->Run();
	delete app;

	return 1;
}

#endif 
