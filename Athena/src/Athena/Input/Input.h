#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"
#include "Keyboard.h"
#include "Mouse.h"


namespace Athena
{
	class ATHENA_API Input
	{
	public:
		static bool IsKeyPressed(Keyboard::Key keycode);
		static bool IsMouseButtonPressed(Mouse::Button button);
		static Vector2 GetMousePosition();
	};
}
