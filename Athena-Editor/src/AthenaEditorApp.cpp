#include <Athena.h>
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
			PushLayer(new EditorLayer());
			ATN_WARN(Min(1.f, 2.f, 3.f, 4.f));
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

