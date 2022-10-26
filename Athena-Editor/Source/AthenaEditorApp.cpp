#include "Athena/Core/Application.h"
#include <Athena/Core/EntryPoint.h>

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
		auto test = std::filesystem::current_path();
		ApplicationDescription appdesc;
		appdesc.WindowDesc.Width = 1600;
		appdesc.WindowDesc.Height = 900;
		appdesc.WindowDesc.Title = "Athena Editor";
		appdesc.WindowDesc.VSync = true;
		appdesc.WindowDesc.Mode = WindowMode::Maximized;
		appdesc.WindowDesc.Icon = "Resources/Icons/Logo/no-background";

#ifdef FORCE_GLFW
		appdesc.API = RendererAPI::OpenGL;
#else
		appdesc.API = RendererAPI::Direct3D;
#endif
		appdesc.UseConsole = true;
		appdesc.UseImGui = true;
		appdesc.WorkingDirectory = Filepath();
		appdesc.ScriptsFolder = "Assets/Scripts";

		return new AthenaEditor(appdesc);
	}
}
