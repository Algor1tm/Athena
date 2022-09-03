#include "atnpch.h"

#include "Athena/Input/Input.h"
#include "Athena/Core/Application.h"

#include <GLFW/glfw3.h>


namespace Athena
{
	bool Input::IsKeyPressed(Keyboard::Key keycode)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetKey(window, keycode);
		return state == GLFW_PRESS;
	}

	bool Input::IsMouseButtonPressed(Mouse::Button button)
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
