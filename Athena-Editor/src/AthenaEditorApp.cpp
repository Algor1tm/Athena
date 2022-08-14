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

			Quat quat = RotateQuat(Radians(45.f), Vector3(0, 0, 1.f));
			Matrix4 mat = RotateMatrix(Radians(45.f), Vector3(0, 0, 1.f));
			ATN_WARN("{0}", mat);
			ATN_WARN("{0}", Matrix4(quat));
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
