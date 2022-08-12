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
			PushLayer(new EditorLayer);

			HEXColor color;
			ATN_WARN("{0}", color);
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
