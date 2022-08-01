#include <Athena.h>
#include <Athena/Core/EntryPoint.h>

#include "SandBox2D.h"


class SandBox: public Athena::Application
{
public:
	SandBox() 
	{
		PushLayer(new SandBox2D());
		Athena::Vector4 v(-1);
		v.Apply(std::abs).Apply(std::sqrt);
	}
	
	~SandBox()
	{

	}
};


Athena::Application* Athena::CreateApplication()
{
	return new SandBox();
}
