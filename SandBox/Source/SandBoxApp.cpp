#include <EntryPoint.h>
#include "Athena/Core/Application.h"

#include "SandBox2D.h"

using namespace Athena;


class SandBox : public Application
{
public:
	SandBox(const ApplicationDescription& appdesc)
		: Application(appdesc)
	{
		PushLayer(CreateRef<SandBox2D>());
	}

	~SandBox()
	{

	}
};


namespace Athena
{
	Application* CreateApplication()
	{
		ApplicationDescription appdesc;
		appdesc.WindowDesc.Width = 1600;
		appdesc.WindowDesc.Height = 900;
		appdesc.WindowDesc.Title = "SandBox";
		appdesc.WindowDesc.VSync = true;
		appdesc.WindowDesc.Mode = WindowMode::Maximized;
		appdesc.WindowDesc.Icon = "Resources/Icons/Logo/no-background";
		appdesc.API = RendererAPI::OpenGL;
		appdesc.UseImGui = false;
		appdesc.UseConsole = false;
		appdesc.WorkingDirectory = "../Athena-Editor";
		appdesc.ScriptsFolder = "Assets/Scripts";

		return new SandBox(appdesc);
	}
}
