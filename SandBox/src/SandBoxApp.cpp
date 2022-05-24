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
		Athena::Vector4 vec4 = Athena::Vector3::forward;
		ATN_INFO(Athena::ToString(vec4));
	}
	 
	~SandBox()
	{

	}
};


Athena::Application* Athena::CreateApplication()
{
	return new SandBox();
}
