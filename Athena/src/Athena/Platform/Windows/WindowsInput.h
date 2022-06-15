#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Input.h"


namespace Athena
{
	class ATHENA_API WindowsInput : public Input
	{
	protected:
		bool IsKeyPressedImpl(KeyCode keycode) override final;
		bool IsMousebuttonPressedImpl(MouseCode button) override final;
		Vector2 GetMouseImpl() override final;
	};
}
