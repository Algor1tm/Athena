#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Application.h"

#include "Athena/Input/Input.h"

#include <GLFW/glfw3.h>


namespace Athena
{
	bool Input::IsKeyPressed(Keyboard::Key keycode)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetKey(window, Input::ConvertToNativeKeyCode(keycode));
		return state == GLFW_PRESS;
	}

	bool Input::IsMouseButtonPressed(Mouse::Button button)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetMouseButton(window, Input::ConvertToNativeMouseCode(button));
		return state;
	}

	Vector2 Input::GetMousePosition()
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		return Vector2((float)x, float(y));
	}

	int32 Input::ConvertToNativeKeyCode(Keyboard::Key keycode)
	{
		return (int32)keycode;
	}

	Keyboard::Key Input::ConvertFromNativeKeyCode(int32 keycode)
	{
		return (Keyboard::Key)keycode;
	}

	int32 Input::ConvertToNativeMouseCode(Mouse::Button keycode)
	{
		return (int32)keycode;
	}

	Mouse::Button Input::ConvertFromNativeMouseCode(int32 keycode)
	{
		return (Mouse::Button)keycode;
	}
}
