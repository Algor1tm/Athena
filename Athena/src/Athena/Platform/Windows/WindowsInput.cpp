#include "atnpch.h"

#include "Athena/Core/Input.h"
#include "Athena/Core/Application.h"

#include <GLFW/glfw3.h>


namespace Athena
{
	bool Input::IsKeyPressed(KeyCode keycode)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetKey(window, keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonPressed(MouseCode button)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetMouseButton(window, button);
		return state;
	}

	Vector2 Input::GetMousePosition()
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		return Vector2((float)x, float(y));
	}
}
