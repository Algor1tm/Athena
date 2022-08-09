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
			Vector4 v(1.f, 2.f, 0.5f, 0.001f);
			ATN_WARN(Log2(16.f));
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

