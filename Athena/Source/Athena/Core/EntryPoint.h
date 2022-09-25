#pragma once

#include "Core.h"
#include "Application.h"


extern Athena::Application* Athena::CreateApplication();

#ifndef ATN_PLATFORM_WINDOWS

int main(int argc, char** argv)
{
    Athena::Application* app = Athena::CreateApplication();

    app->Run();
    delete app;

    return EXIT_SUCCESS;
}
#else

#include <Windows.h>


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)

{
    Athena::Application* app = Athena::CreateApplication();

    app->Run();
    delete app;

    return EXIT_SUCCESS;
}


#endif 
