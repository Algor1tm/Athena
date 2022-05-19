#include <Athena.h>


class SandBox: public Athena::Application
{
public:
	SandBox() 
	{
		PushOverlay(new Athena::ImGuiLayer());
	}

	~SandBox()
	{

	}
};


Athena::Application* Athena::CreateApplication()
{
	return new SandBox();
}
