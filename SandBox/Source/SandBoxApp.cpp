#include "Athena/Core/Application.h"
#include <Athena/Core/EntryPoint.h>

#include "SandBox2D.h"

#include "Athena/Core/PlatformUtils.h"
#include "Athena/Renderer/ConstantBuffer.h"

#include <ImGui/imgui.h>


namespace Athena
{
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


	Application* CreateApplication()
	{
		ApplicationDescription appdesc;
		appdesc.WindowDesc.Width = 1600;
		appdesc.WindowDesc.Height = 900;
		appdesc.WindowDesc.Title = "SandBox";
		appdesc.WindowDesc.VSync = true;
		appdesc.WindowDesc.Mode = WindowMode::Maximized;
		appdesc.WindowDesc.Icon = "Resources/Icons/Logo/no-background";

#ifdef FORCE_GLFW
		appdesc.API = RendererAPI::OpenGL;
#else
		appdesc.API = RendererAPI::Direct3D;
#endif
		appdesc.UseImGui = false;
		appdesc.UseConsole = true;
		appdesc.WorkingDirectory = "../Athena-Editor";
		appdesc.ScriptsFolder = "Assets/Scripts";

		return new SandBox(appdesc);
	}
}
