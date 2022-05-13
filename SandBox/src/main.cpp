#include <Athena.h>


class SandBox: public Athena::Application
{
public:
	SandBox() 
	{

	}

	~SandBox()
	{

	}
};


Athena::Application* Athena::CreateApplication()
{
	return new SandBox();
}
