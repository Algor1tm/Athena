#pragma once

#include "Athena/Input/Input.h"
#include "Athena/Core/Application.h"

#include <Windows.h>


namespace Athena
{
	static inline int AthenaKeyCodeToWinKeyCode(Keyboard::Key athenaKeyCode)
	{
		return static_cast<int>(athenaKeyCode);
	}

	static inline int AthenaMouseCodeToWinMouseCode(Mouse::Button athenaMouseCode)
	{
		return static_cast<int>(athenaMouseCode);
	}


	bool Input::IsKeyPressed(Keyboard::Key keycode)
	{
		return GetAsyncKeyState(AthenaKeyCodeToWinKeyCode(keycode)) < 0;
	}

	bool Input::IsMouseButtonPressed(Mouse::Button button)
	{
		return GetAsyncKeyState(AthenaMouseCodeToWinMouseCode(button)) < 0;
	}

	Vector2 Input::GetMousePosition()
	{
		auto window = reinterpret_cast<HWND>(Application::Get().GetWindow().GetNativeWindow());
		POINT cursor;
		if (GetCursorPos(&cursor))
		{
			if (ScreenToClient(window, &cursor))
			{
				return Vector2((float)cursor.x, (float)cursor.y);
			}
		}

		ATN_CORE_ERROR("Failed to get mouse position!");
		return Vector2(0, 0);
	}
}
