#include "Athena/Core/Application.h"
#include <Athena/Core/EntryPoint.h>

#include "SandBox2D.h"

namespace Athena
{
	class SandBox : public Application
	{
	public:
		SandBox(const ApplicationDESC& appdesc)
			: Application(appdesc)
		{
			PushLayer(new SandBox2D());
		}

		~SandBox()
		{

		}
	};


	Application* CreateApplication()
	{
		ApplicationDESC appdesc;
		appdesc.WindowWidth = 1600;
		appdesc.WindowHeight = 900;
		appdesc.Title = "SandBox";
		appdesc.VSync = true;

		appdesc.UseImGui = false;
		appdesc.WorkingDirectory = "../Athena-Editor";

		return new SandBox(appdesc);
	}
}
