#pragma once

#include "Core.h"
#include "KeyCodes.h"
#include "MouseCodes.h"
#include "Athena/Math/Vector2.h"


namespace Athena
{
	class ATHENA_API Input
	{
	public:
		inline static bool IsKeyPressed(KeyCode keycode) { return s_Instance->IsKeyPressedImpl(keycode); }
		inline static bool IsMouseButtonPressed(MouseCode button) { return s_Instance->IsMousebuttonPressedImpl(button); }
		inline static Vector2 GetMouse() { return s_Instance->GetMouseImpl(); }

	protected:
		virtual bool IsKeyPressedImpl(KeyCode keycode) = 0;
		virtual bool IsMousebuttonPressedImpl(MouseCode button) = 0;
		virtual Vector2 GetMouseImpl() = 0;

	private:
		static Input* s_Instance;
	};
}
