#include "Athena/Core/Application.h"
#include <Athena/Core/EntryPoint.h>

#include "EditorLayer.h"


namespace Athena
{
	class AthenaEditor : public Application
	{
	public:
		AthenaEditor(const WindowDesc& wdesc)
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
		WindowDesc wdesc;
		wdesc.Title = "Athena Editor";
		return new AthenaEditor(wdesc);
	}
}
