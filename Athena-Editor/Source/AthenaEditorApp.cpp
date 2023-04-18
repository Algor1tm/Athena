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
			PushLayer(CreateRef<EditorLayer>());
		}

		~AthenaEditor()
		{
			
		}
	};


	Application* CreateApplication()
	{
		ApplicationDescription appdesc;
		appdesc.WindowDesc.Width = 1600;
		appdesc.WindowDesc.Height = 900;
		appdesc.WindowDesc.Title = "Athena Editor";
		appdesc.WindowDesc.VSync = true;
		appdesc.WindowDesc.Mode = WindowMode::Maximized;
		appdesc.WindowDesc.Icon = "Resources/Icons/Logo/no-background";
		appdesc.API = Renderer::API::OpenGL;
		appdesc.EnableConsole = true;
		appdesc.EnableImGui = true;
		appdesc.WorkingDirectory = FilePath();
		appdesc.ScriptsFolder = "Assets/Scripts";

		return new AthenaEditor(appdesc);
	}
}
