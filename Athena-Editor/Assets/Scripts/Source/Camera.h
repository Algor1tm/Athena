#pragma once

#include <Athena.h>

using namespace Athena;


namespace TestProject
{
	class Camera: public Script
	{
	public:
		Camera();

		virtual void OnCreate() override;
		virtual void OnUpdate(Time frameTime) override;

	private:

	};
}
