#include <Athena.h>


class ExampleLayer : public Athena::Layer
{
public:
	ExampleLayer()
		: Layer("ExampleLayer") {}

	void OnUpdate() override
	{
		ATN_INFO("ExampleLayer::Update");
	}

	void OnEvent(Athena::Event& event) override
	{
		ATN_TRACE("{0}", event.ToString());
	}
};


class SandBox: public Athena::Application
{
public:
	SandBox() 
	{
		PushLayer(new ExampleLayer());
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
