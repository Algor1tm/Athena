#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Log.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Window.h"

#include "Athena/Input/ApplicationEvent.h"
#include "Athena/Input/KeyEvent.h"
#include "Athena/Input/MouseEvent.h"

#include  "Athena/Platform/OpenGL/GLGraphicsContext.h"

#include <GLFW/glfw3.h>
#include <stb_image/stb_image.h>


namespace Athena
{
	static inline Keyboard::Key GLFWKeyCodeToAthenaKeyCode(int glfwKeyCode);
	static inline Mouse::Button GLFWMouseCodeToAthenaMouseCode(int glfwMouseCode);


	static void GLFWErrorCallback(int error, const char* description)
	{
		ATN_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	static void SetEventCallbacks(GLFWwindow* windowHandle)
	{
		glfwSetWindowCloseCallback(windowHandle, [](GLFWwindow* window)
			{
				Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));

		WindowCloseEvent event;
		data.EventCallback(event);
			});

		glfwSetWindowSizeCallback(windowHandle, [](GLFWwindow* window, int width, int height)
			{
				Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
		data.Width = width;
		data.Height = height;

		WindowResizeEvent event(width, height);
		data.EventCallback(event);
			});

		glfwSetWindowIconifyCallback(windowHandle, [](GLFWwindow* window, int iconified)
			{
				Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
		if (iconified)
			data.Mode = WindowMode::Minimized;
		else
			data.Mode = WindowMode::Default;
			});

		glfwSetWindowMaximizeCallback(windowHandle, [](GLFWwindow* window, int maximized)
			{
				Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));
		if (maximized)
			data.Mode = WindowMode::Minimized;
		else
			data.Mode = WindowMode::Default;
			});

		glfwSetKeyCallback(windowHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));

		switch (action)
		{
		case GLFW_PRESS:
		{
			KeyPressedEvent event(GLFWKeyCodeToAthenaKeyCode(key), false);
			data.EventCallback(event);
			break;
		}
		case GLFW_RELEASE:
		{
			KeyReleasedEvent event(GLFWKeyCodeToAthenaKeyCode(key));
			data.EventCallback(event);
			break;
		}
		case GLFW_REPEAT:
		{
			KeyPressedEvent event(GLFWKeyCodeToAthenaKeyCode(key), true);
			data.EventCallback(event);
			break;
		}
		}
			});

		glfwSetCharCallback(windowHandle, [](GLFWwindow* window, unsigned int character)
			{
				Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));

		KeyTypedEvent event(static_cast<Keyboard::Key>(UnicodeToASCII(character)));
		data.EventCallback(event);
			});

		glfwSetMouseButtonCallback(windowHandle, [](GLFWwindow* window, int button, int action, int modes)
			{
				Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));

		switch (action)
		{
		case GLFW_PRESS:
		{
			MouseButtonPressedEvent event(GLFWMouseCodeToAthenaMouseCode(button));
			data.EventCallback(event);
			break;
		}
		case GLFW_RELEASE:
		{
			MouseButtonReleasedEvent event(GLFWMouseCodeToAthenaMouseCode(button));
			data.EventCallback(event);
			break;
		}
		}
			});

		glfwSetScrollCallback(windowHandle, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(windowHandle, [](GLFWwindow* window, double x, double y)
			{
				Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(glfwGetWindowUserPointer(window));

				MouseMovedEvent event((float)x, (float)y);
				data.EventCallback(event);
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
		windowData.EventCallback = info.EventCallback;

		window->m_Data = windowData;

		if (m_WindowCount == 0)
		{
			int success = glfwInit();
			ATN_CORE_ASSERT(success, "Could not intialize GLFW");
			ATN_CORE_INFO("Init GLFW");

			glfwWindowHint(GLFW_TITLEBAR, info.CustomTitlebar ? GLFW_FALSE : GLFW_TRUE);

			glfwSetErrorCallback(GLFWErrorCallback);
		}

		GLFWwindow* hWnd;
		window->m_WindowHandle = hWnd = glfwCreateWindow(
			window->m_Data.Width,
			window->m_Data.Height,
			window->m_Data.Title.c_str(),
			nullptr,
			nullptr);

		m_WindowCount++;

		ATN_CORE_INFO("Create GLFW Window '{0}' ({1}, {2})", window->m_Data.Title, window->m_Data.Width, window->m_Data.Height);

		glfwSetWindowAttrib(hWnd, GLFW_RESIZABLE, info.WindowResizeable ? GLFW_TRUE : GLFW_FALSE);

		glfwSetWindowUserPointer(hWnd, &window->m_Data);
		SetEventCallbacks(hWnd);

		window->m_Context = CreateRef<GLGraphicsContext>(reinterpret_cast<GLFWwindow*>(window->m_WindowHandle));;
		window->SetVSync(window->m_Data.VSync);

		window->SetWindowMode(info.Mode);

		if (FileSystem::Exists(info.Icon))
		{
			GLFWimage image;
			image.pixels = stbi_load(info.Icon.string().c_str(), &image.width, &image.height, 0, 4);
			if (image.pixels)
			{
				glfwSetWindowIcon(hWnd, 1, &image);
				stbi_image_free(image.pixels);
			}
			else
			{
				ATN_CORE_ERROR("GLFWwindow: failed to load icon from {}!", info.Icon);
			}
		}
		else if(!info.Icon.empty())
		{
			ATN_CORE_ERROR("GLFWwindow: invalid filepath for icon '{}'!", info.Icon);
		}

		return window;
	}

	Window::~Window()
	{
		glfwDestroyWindow(reinterpret_cast<GLFWwindow*>(m_WindowHandle));
		--m_WindowCount;

		ATN_CORE_INFO("Shutdown GLFW Window '{0}'", m_Data.Title);

		if (m_WindowCount <= 0)
		{
			glfwTerminate();
			ATN_CORE_INFO("Shutdown GLFW");
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
			glfwSetWindowMonitor(hWnd, nullptr, 100, 100, m_Data.Width * 3 / 4, m_Data.Width * 3 / 4, mode->refreshRate);
		}

		switch (mode)
		{
		case WindowMode::Default:
		{
			break;
		}
		case WindowMode::Maximized:
		{
			int area_x, area_y, area_width, area_height;
			glfwGetMonitorWorkarea(monitor, &area_x, &area_y, &area_width, &area_height);
			m_Data.Width = area_width;
			m_Data.Height = area_height;
			glfwSetWindowPos(hWnd,
				area_x,
				area_y);
			
			glfwSetWindowSize(hWnd, m_Data.Width, m_Data.Height);
			glfwMaximizeWindow(hWnd);
			glfwShowWindow(hWnd);

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


	static inline Keyboard::Key GLFWKeyCodeToAthenaKeyCode(int glfwKeyCode)
	{
		switch (glfwKeyCode)
		{
		case 32: return Keyboard::Space;
		case 39: return Keyboard::Apostrophe;
		case 44: return Keyboard::Comma;
		case 45: return Keyboard::Minus;
		case 46: return Keyboard::Dot;
		case 47: return Keyboard::Slash;

		case 48: return Keyboard::D0;
		case 49: return Keyboard::D1;
		case 50: return Keyboard::D2;
		case 51: return Keyboard::D3;
		case 52: return Keyboard::D4;
		case 53: return Keyboard::D5;
		case 54: return Keyboard::D6;
		case 55: return Keyboard::D7;
		case 56: return Keyboard::D8;
		case 57: return Keyboard::D9;

		case 59: return Keyboard::Semicolon;
		case 61: return Keyboard::Equal;

		case 65: return Keyboard::A;
		case 66: return Keyboard::B;
		case 67: return Keyboard::C;
		case 68: return Keyboard::D;
		case 69: return Keyboard::E;
		case 70: return Keyboard::F;
		case 71: return Keyboard::G;
		case 72: return Keyboard::H;
		case 73: return Keyboard::I;
		case 74: return Keyboard::J;
		case 75: return Keyboard::K;
		case 76: return Keyboard::L;
		case 77: return Keyboard::M;
		case 78: return Keyboard::N;
		case 79: return Keyboard::O;
		case 80: return Keyboard::P;
		case 81: return Keyboard::Q;
		case 82: return Keyboard::R;
		case 83: return Keyboard::S;
		case 84: return Keyboard::T;
		case 85: return Keyboard::U;
		case 86: return Keyboard::V;
		case 87: return Keyboard::W;
		case 88: return Keyboard::X;
		case 89: return Keyboard::Y;
		case 90: return Keyboard::Z;

		case 91: return Keyboard::LeftBracket;
		case 92: return Keyboard::Backslash;
		case 93: return Keyboard::RightBracket;
		case 96: return Keyboard::GraveAccent;

		case 256: return Keyboard::Escape;
		case 257: return Keyboard::Enter;
		case 258: return Keyboard::Tab;
		case 259: return Keyboard::Backspace;
		case 260: return Keyboard::Insert;
		case 261: return Keyboard::Delete;
		case 262: return Keyboard::Right;
		case 263: return Keyboard::Left;
		case 264: return Keyboard::Down;
		case 265: return Keyboard::Up;
		case 266: return Keyboard::PageUp;
		case 267: return Keyboard::PageDown;
		case 268: return Keyboard::Home;
		case 269: return Keyboard::End;
		case 280: return Keyboard::CapsLock;
		case 281: return Keyboard::ScrollLock;
		case 282: return Keyboard::NumLock;
		case 283: return Keyboard::PrintScreen;
		case 284: return Keyboard::Pause;
		case 290: return Keyboard::F1;
		case 291: return Keyboard::F2;
		case 292: return Keyboard::F3;
		case 293: return Keyboard::F4;
		case 294: return Keyboard::F5;
		case 295: return Keyboard::F6;
		case 296: return Keyboard::F7;
		case 297: return Keyboard::F8;
		case 298: return Keyboard::F9;
		case 299: return Keyboard::F10;
		case 300: return Keyboard::F11;
		case 301: return Keyboard::F12;
		case 302: return Keyboard::F13;
		case 303: return Keyboard::F14;
		case 304: return Keyboard::F15;
		case 305: return Keyboard::F16;
		case 306: return Keyboard::F17;
		case 307: return Keyboard::F18;
		case 308: return Keyboard::F19;
		case 309: return Keyboard::F20;
		case 310: return Keyboard::F21;
		case 311: return Keyboard::F22;
		case 312: return Keyboard::F23;
		case 313: return Keyboard::F24;

		case 320: return Keyboard::KP0;
		case 321: return Keyboard::KP1;
		case 322: return Keyboard::KP2;
		case 323: return Keyboard::KP3;
		case 324: return Keyboard::KP4;
		case 325: return Keyboard::KP5;
		case 326: return Keyboard::KP6;
		case 327: return Keyboard::KP7;
		case 328: return Keyboard::KP8;
		case 329: return Keyboard::KP9;
		case 330: return Keyboard::KPDecimal;
		case 331: return Keyboard::KPDivide;
		case 332: return Keyboard::KPMultiply;
		case 333: return Keyboard::KPSubtract;
		case 334: return Keyboard::KPAdd;
		case 335: return Keyboard::KPEnter;

		case 340: return Keyboard::LShift;
		case 341: return Keyboard::LCtrl;
		case 342: return Keyboard::LAlt;
		case 343: return Keyboard::LWindows;
		case 344: return Keyboard::RShift;
		case 345: return Keyboard::RCtrl;
		case 346: return Keyboard::RAlt;
		case 347: return Keyboard::RWindows;
		case 348: return Keyboard::Menu;
		}

		ATN_CORE_ERROR("Failed to match GLFW KeyCode with Athena KeyCode '{0}'", glfwKeyCode);
		return Keyboard::Space;
	}

	static inline Mouse::Button GLFWMouseCodeToAthenaMouseCode(int glfwMouseCode)
	{
		switch (glfwMouseCode)
		{
		case 0: return Mouse::Left;
		case 1: return Mouse::Right;
		case 2: return Mouse::Middle;
		case 3: return Mouse::XButton1;
		case 4: return Mouse::XButton2;
		}

		ATN_CORE_ERROR("Failed to match GLFW MouseCode with Athena MouseCode '{0}'", glfwMouseCode);
		return Mouse::XButton1;
	}
}
