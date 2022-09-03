#include "Athena/Core/Application.h"
#include <Athena/Core/EntryPoint.h>

#include "EditorLayer.h"


namespace Athena
{
	class AthenaEditor : public Application
	{
	public:
		AthenaEditor(const WindowDESC& wdesc)
			: Application(wdesc)
		{
			PushLayer(new EditorLayer);
		}

		~AthenaEditor()
		{

		}
	};


	Application* CreateApplication()
	{
		WindowDESC wdesc;
		wdesc.Title = "Athena Editor";
		wdesc.VSync = true;
		return new AthenaEditor(wdesc);
	}
}
