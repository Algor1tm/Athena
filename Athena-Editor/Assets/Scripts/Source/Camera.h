#pragma once

#include <Athena.h>

using namespace Athena;


namespace TestProject
{
	class Camera: public Script
	{
	public:
		Camera();

	private:
		virtual void OnCreate() override;
		virtual void OnUpdate(Time frameTime) override;
		virtual void GetFieldsDescription(ScriptFieldMap* outFields) override;

	private:
		float m_Speed = 1.f;
		bool m_Checkbox = true;
	};
}
