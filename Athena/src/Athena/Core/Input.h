#pragma once

#include "Core.h"
#include "KeyCodes.h"
#include "MouseCodes.h"
#include "Athena/Math/Vector.h"


namespace Athena
{
	class ATHENA_API Input
	{
	public:
		static bool IsKeyPressed(KeyCode keycode);
		static bool IsMouseButtonPressed(MouseCode button);
		static Vector2 GetMousePosition();
	};
}
