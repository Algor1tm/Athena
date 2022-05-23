#pragma once

#include "Athena/Core.h"
#include "Athena/Input.h"


namespace Athena
{
	class ATHENA_API WindowsInput : public Input
	{
	protected:
		bool IsKeyPressedImpl(KeyCode keycode) override final;
		bool IsMousebuttonPressedImpl(MouseCode button) override final;
		float GetMouseXImpl() override final;
		float GetMouseYImpl() override final;
	};
}
