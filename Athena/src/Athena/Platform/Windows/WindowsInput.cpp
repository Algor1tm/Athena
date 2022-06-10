#include "atnpch.h"

#include "GLFW/glfw3.h"

#include "WindowsInput.h"
#include "Athena/Core/Application.h"


namespace Athena
{
	Input* Input::s_Instance = new WindowsInput();

	bool WindowsInput::IsKeyPressedImpl(KeyCode keycode)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetKey(window, keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}


	bool WindowsInput::IsMousebuttonPressedImpl(MouseCode button)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetMouseButton(window, button);
		return state;
	}


	Vector2 WindowsInput::GetMouseImpl()
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		return Vector2((float)x, float(y));
	}
}
