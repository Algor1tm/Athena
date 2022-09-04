#include "Athena/Core/Application.h"
#include <Athena/Core/EntryPoint.h>

#include "EditorLayer.h"


namespace Athena
{
	class AthenaEditor : public Application
	{
	public:
		AthenaEditor(const ApplicationDESC& appdesc)
			: Application(appdesc)
		{
			PushLayer(new EditorLayer);
		}

		~AthenaEditor()
		{

		}
	};


	Application* CreateApplication()
	{
		ApplicationDESC appdesc;
		appdesc.WindowWidth = 1600;
		appdesc.WindowHeight = 900;
		appdesc.Title = "Athena Editor";
		appdesc.VSync = true;
		appdesc.UseImGui = true;

		return new AthenaEditor(appdesc);
	}
}
