#include <EntryPoint.h>
#include "Athena/Core/Application.h"

#include "SandBoxLayer.h"

using namespace Athena;


class SandBox : public Application
{
public:
	SandBox(const ApplicationDescription& appdesc)
		: Application(appdesc)
	{

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

		appdesc.AppConfig.EnableImGui = false;
		appdesc.AppConfig.EnableConsole = false;
		appdesc.AppConfig.WorkingDirectory = "../Athena-Editor";

		appdesc.RendererConfig.API = Renderer::API::OpenGL;
		appdesc.RendererConfig.ShaderPack = "../Athena/EngineResources/Shaders";

		appdesc.ScriptConfig.ScriptsFolder = "Assets/Scripts";

		appdesc.WindowDesc.Width = 1600;
		appdesc.WindowDesc.Height = 900;
		appdesc.WindowDesc.Title = "SandBox";
		appdesc.WindowDesc.VSync = true;
		appdesc.WindowDesc.Mode = WindowMode::Maximized;
		appdesc.WindowDesc.Icon = "EditorResources/Icons/Logo/no-background";

		Application* app = new SandBox(appdesc);


		FilePath scene = "Assets/Scenes/Sponza.atn";

		app->PushLayer(CreateRef<SandBoxLayer>(scene));

		return app;
	}
}
