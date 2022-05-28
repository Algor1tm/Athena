#include <Athena.h>


class SandBox: public Athena::Application
{
public:
	SandBox() 
	{
		PushOverlay(new Athena::ImGuiLayer());
		Athena::Vector<float, 1> v{1900};
		Athena::Vector2i vec(0);
		Athena::Vector<int, 1> t(std::move(v));
		Athena::Vector4 vec4 = Athena::Vector3::forward;
		vec4.Apply(std::abs);
		vec4 += Athena::Vector3::right;
		for(auto elem: vec4)
			ATN_INFO(elem);

		Athena::Matrix<float, 1, 4> mat(0);
		Athena::Matrix<int, 5, 5> m(-1);
		for(auto& t: m)
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
