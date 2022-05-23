#include <Athena.h>


class SandBox: public Athena::Application
{
public:
	SandBox() 
	{
		PushOverlay(new Athena::ImGuiLayer());
		Athena::Vector<float, 1> v{1900};
		Athena::Vector2i vec;
		Athena::Vector<int, 1> t(std::move(v));
		ATN_INFO(Athena::ToString(t));
	}

	~SandBox()
	{

	}
};


Athena::Application* Athena::CreateApplication()
{
	return new SandBox();
}
