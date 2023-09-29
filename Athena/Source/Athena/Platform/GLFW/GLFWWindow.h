#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Window.h"

#include "Athena/Input/Input.h"
#include "Athena/Input/WindowEvent.h"
#include "Athena/Input/KeyEvent.h"
#include "Athena/Input/MouseEvent.h"

#include  "Athena/Platform/OpenGL/GLGraphicsContext.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <stb_image/stb_image.h>


namespace Athena
{
	static void GLFWErrorCallback(int error, const char* description)
	{
		ATN_CORE_ERROR_TAG("GLFWWindow", "Error({0}) : {1}", error, description);
	}

	static Window::WindowData& GetUserPointer(GLFWwindow* window)
	{
		return *reinterpret_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
	}

	static void SetEventCallbacks(GLFWwindow* windowHandle)
	{
		glfwSetWindowCloseCallback(windowHandle, [](GLFWwindow* window)
			{
				Window::WindowData& data = GetUserPointer(window);

				Ref<WindowCloseEvent> event = CreateRef<WindowCloseEvent>();
				data.EventCallback(event);
			});

		glfwSetWindowSizeCallback(windowHandle, [](GLFWwindow* window, int width, int height)
			{
				Window::WindowData& data = GetUserPointer(window);
				data.Width = width;
				data.Height = height;

				Ref<WindowResizeEvent> event = CreateRef<WindowResizeEvent>(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowPosCallback(windowHandle, [](GLFWwindow* window, int xpos, int ypos)
			{
				Window::WindowData& data = GetUserPointer(window);
				
				Ref<WindowMoveEvent> event = CreateRef<WindowMoveEvent>(xpos, ypos);
				data.EventCallback(event);
			});

		glfwSetWindowFocusCallback(windowHandle, [](GLFWwindow* window, int focused)
			{
				Window::WindowData& data = GetUserPointer(window);

				if (focused)
				{
					Ref<WindowGainedFocusEvent> event = CreateRef<WindowGainedFocusEvent>();
					data.EventCallback(event);
				}
				else
				{
					Ref<WindowLostFocusEvent> event = CreateRef<WindowLostFocusEvent>();
					data.EventCallback(event);
				}
			});

		glfwSetWindowMaximizeCallback(windowHandle, [](GLFWwindow* window, int maximized)
			{
				Window::WindowData& data = GetUserPointer(window);
				if (maximized)
				{
					data.Mode = WindowMode::Maximized;

					Ref<WindowMaximizeEvent> event = CreateRef<WindowMaximizeEvent>();
					data.EventCallback(event);
				}
				else
				{
					data.Mode = WindowMode::Default;

					Ref<WindowRestoreEvent> event = CreateRef<WindowRestoreEvent>();
					data.EventCallback(event);
				}
			});

		glfwSetWindowIconifyCallback(windowHandle, [](GLFWwindow* window, int iconified)
			{
				Window::WindowData& data = GetUserPointer(window);
				if (iconified)
				{
					data.Mode = WindowMode::Minimized;

					Ref<WindowIconifyEvent> event = CreateRef<WindowIconifyEvent>();
					data.EventCallback(event);
				}
				else
				{
					data.Mode = WindowMode::Default;

					Ref<WindowRestoreEvent> event = CreateRef<WindowRestoreEvent>();
					data.EventCallback(event);
				}
			});

		glfwSetKeyCallback(windowHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				Window::WindowData& data = GetUserPointer(window);
				
				bool ctrl = mods & GLFW_MOD_CONTROL;
				bool alt = mods & GLFW_MOD_ALT;
				bool shift = mods & GLFW_MOD_SHIFT;

				switch (action)
				{
				case GLFW_PRESS:
				{
					Ref<KeyPressedEvent> event = CreateRef<KeyPressedEvent>(Input::ConvertFromNativeKeyCode(key), false, ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Ref<KeyReleasedEvent> event = CreateRef<KeyReleasedEvent>(Input::ConvertFromNativeKeyCode(key), ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					Ref<KeyPressedEvent> event = CreateRef<KeyPressedEvent>(Input::ConvertFromNativeKeyCode(key), true, ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetCharCallback(windowHandle, [](GLFWwindow* window, unsigned int character)
			{
				const auto convertFromUnicode = [](unsigned int code) { return (int)code >= 97 ? (int)code - 32 : (int)code; };

				Window::WindowData& data = GetUserPointer(window);

				Ref<KeyTypedEvent> event = CreateRef<KeyTypedEvent>(Input::ConvertFromNativeKeyCode(convertFromUnicode(character)));
				data.EventCallback(event);
			});

		glfwSetMouseButtonCallback(windowHandle, [](GLFWwindow* window, int button, int action, int mods)
			{
				Window::WindowData& data = GetUserPointer(window);

				bool ctrl = mods & GLFW_MOD_CONTROL;
				bool alt = mods & GLFW_MOD_ALT;
				bool shift = mods & GLFW_MOD_SHIFT;

				switch (action)
				{
				case GLFW_PRESS:
				{
					Ref<MouseButtonPressedEvent> event = CreateRef<MouseButtonPressedEvent>(Input::ConvertFromNativeMouseCode(button), ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					Ref<MouseButtonReleasedEvent> event = CreateRef<MouseButtonReleasedEvent>(Input::ConvertFromNativeMouseCode(button), ctrl, alt, shift);
					data.EventCallback(event);
					break;
				}
				}
			});
		
		glfwSetScrollCallback(windowHandle, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				Window::WindowData& data = GetUserPointer(window);

				Ref<MouseScrolledEvent> event = CreateRef<MouseScrolledEvent>((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(windowHandle, [](GLFWwindow* window, double x, double y)
			{
				Window::WindowData& data = GetUserPointer(window);

				Ref<MouseMoveEvent> event = CreateRef<MouseMoveEvent>((float)x, (float)y);
				data.EventCallback(event);
			});

		glfwSetTitlebarHitTestCallback(windowHandle, [](GLFWwindow* window, int x, int y, int* hit)
			{
				Window::WindowData& data = GetUserPointer(window);
		
				if (data.CustomTitlebar)
				{
					*hit = data.TitlebarHitTest();
				}
			});
	}


	Scope<Window> Window::Create(const WindowCreateInfo& info)
	{
		Scope<Window> window = CreateScope<Window>();

		WindowData windowData;
		windowData.Width = info.Width;
		windowData.Height = info.Height;
		windowData.VSync = info.VSync;
		windowData.Title = info.Title;
		windowData.CustomTitlebar = info.CustomTitlebar;

		window->m_Data = windowData;

		if (m_WindowCount == 0)
		{
			int success = glfwInit();
			ATN_CORE_VERIFY(success, "Could not intialize GLFW");
			ATN_CORE_INFO_TAG("GLFWWindow", "Init GLFW");

			glfwWindowHint(GLFW_TITLEBAR, info.CustomTitlebar ? GLFW_FALSE : GLFW_TRUE);

			glfwSetErrorCallback(GLFWErrorCallback);
		}

		GLFWwindow* glfwWindow;
		window->m_WindowHandle = glfwWindow = glfwCreateWindow(
			window->m_Data.Width,
			window->m_Data.Height,
			window->m_Data.Title.c_str(),
			nullptr,
			nullptr);

		m_WindowCount++;

		ATN_CORE_INFO_TAG("GLFWWindow", "Create GLFW Window '{0}' ({1}, {2})", window->m_Data.Title, window->m_Data.Width, window->m_Data.Height);

		glfwSetWindowAttrib(glfwWindow, GLFW_RESIZABLE, info.WindowResizeable ? GLFW_TRUE : GLFW_FALSE);

		glfwSetWindowUserPointer(glfwWindow, &window->m_Data);
		SetEventCallbacks(glfwWindow);

		window->m_Context = CreateRef<GLGraphicsContext>(reinterpret_cast<GLFWwindow*>(window->m_WindowHandle));;
		window->SetVSync(window->m_Data.VSync);

		window->SetWindowMode(info.StartMode);

		if (FileSystem::Exists(info.Icon))
		{
			GLFWimage image;
			image.pixels = stbi_load(info.Icon.string().c_str(), &image.width, &image.height, 0, 4);
			if (image.pixels)
			{
				glfwSetWindowIcon(glfwWindow, 1, &image);
				stbi_image_free(image.pixels);
			}
			else
			{
				ATN_CORE_ERROR_TAG("GLFWWindow", "failed to load icon from {}!", info.Icon);
			}
		}
		else if(!info.Icon.empty())
		{
			ATN_CORE_ERROR_TAG("GLFWWindow", "invalid filepath for icon '{}'!", info.Icon);
		}

		if (glfwRawMouseMotionSupported())
		{
			ATN_CORE_INFO_TAG("GLFWWindow", "Raw mouse motion enabled");
			glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}

		return window;
	}

	Window::~Window()
	{
		glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(m_WindowHandle));
		--m_WindowCount;

		ATN_CORE_INFO_TAG("GLFWWindow", "Destroy Window '{0}'", m_Data.Title);

		if (m_WindowCount <= 0)
		{
			glfwTerminate();
			ATN_CORE_INFO_TAG("GLFWWindow", "Shutdown GLFW");
		}
	}

	void Window::OnUpdate()
	{
		glfwPollEvents();
		m_Context->SwapBuffers();
	}

	void Window::SetVSync(bool enabled)
	{
		m_Context->SetVSync(enabled);
		m_Data.VSync = enabled;
	}

	void Window::HideCursor(bool hide)
	{
		if (hide)
			glfwSetInputMode((GLFWwindow*)m_WindowHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode((GLFWwindow*)m_WindowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	void Window::SetCursorPosition(Vector2 position)
	{
		glfwSetCursorPos((GLFWwindow*)m_WindowHandle, position.x, position.y);
	}

	void Window::SetWindowMode(WindowMode mode)
	{
		WindowMode currentMode = GetWindowMode();
		GLFWwindow* hWnd = reinterpret_cast<GLFWwindow*>(m_WindowHandle);
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();

		if (currentMode == mode)
			return;

		if (currentMode == WindowMode::Fullscreen)
		{
			m_Context->SetFullscreen(false);
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(hWnd, nullptr, 0, 0, m_Data.Width, m_Data.Width, mode->refreshRate);
		}

		switch (mode)
		{
		case WindowMode::Default:
		{
			glfwRestoreWindow(hWnd);
			break;
		}
		case WindowMode::Maximized:
		{
			glfwMaximizeWindow(hWnd);
			break;
		}
		case WindowMode::Minimized:
		{
			glfwIconifyWindow(hWnd);
			break;
		}
		case WindowMode::Fullscreen:
		{
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			
			glfwSetWindowMonitor(hWnd, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			m_Context->SetFullscreen(true);
			break;
		}
		}

		m_Data.Mode = mode;
	}
}
