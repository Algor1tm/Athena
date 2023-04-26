#include <EntryPoint.h>
#include "Athena/Core/Application.h"

#include "EditorLayer.h"


namespace Athena
{
	class AthenaEditor : public Application
	{
	public:
		AthenaEditor(const ApplicationDescription& appdesc)
			: Application(appdesc)
		{

		}

		~AthenaEditor()
		{
			
		}
	};


	Application* CreateApplication()
	{
		ApplicationDescription appdesc;

		appdesc.RendererConfig.API = Renderer::API::OpenGL;
		appdesc.RendererConfig.ShaderPack = "../Athena/EngineResources/Shaders";

		appdesc.ScriptConfig.ScriptsFolder = "Assets/Scripts";

		appdesc.WindowDesc.Width = 1600;
		appdesc.WindowDesc.Height = 900;
		appdesc.WindowDesc.Title = "Athena Editor";
		appdesc.WindowDesc.VSync = true;
		appdesc.WindowDesc.Mode = WindowMode::Maximized;
		appdesc.WindowDesc.Icon = "EditorResources/Icons/Logo/no-background.png";

		appdesc.AppConfig.EnableConsole = true;
		appdesc.AppConfig.EnableImGui = true;
		appdesc.AppConfig.WorkingDirectory = FilePath();


		Application* application = new AthenaEditor(appdesc);
		
		EditorConfig editorConfig;
		editorConfig.EditorResources = "EditorResources/";

		application->PushLayer(CreateRef<EditorLayer>(editorConfig));

		return application;
	}
}
