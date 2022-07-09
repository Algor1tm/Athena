#include <Athena.h>
#include <Athena/Core/EntryPoint.h>

#include "SandBox2D.h"


class SandBox: public Athena::Application
{
public:
	SandBox() 
	{
		PushLayer(new SandBox2D());
	}
	
	~SandBox()
	{

	}
};


Athena::Application* Athena::CreateApplication()
{
	return new SandBox();
}
