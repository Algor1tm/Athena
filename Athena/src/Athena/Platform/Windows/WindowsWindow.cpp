#include "atnpch.h"
#include <glad/glad.h>

#include "WindowsWindow.h"
#include "Athena/Log.h"

#include "Athena/Events/ApplicationEvent.h"
#include "Athena/Events/KeyEvent.h"
#include "Athena/Events/MouseEvent.h"


namespace Athena
{
	static bool s_GLFWInitialized = false;

	static void GLFWErrorCallback(int error, const char* description)
	{
		ATN_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	Window* Window::Create(const WindowDesc& desc)
	{
		return new WindowsWindow(desc);
	}


	WindowsWindow::WindowsWindow(const WindowDesc& desc)
		: m_Desc(desc)
	{
		Init();
	}


	WindowsWindow::~WindowsWindow()
	{
		Shutdown();
	}


	void WindowsWindow::Init()
	{
		ATN_CORE_INFO("Creating Window {0} ({1}, {2})", m_Desc.Title, m_Desc.Width, m_Desc.Height);

		if (!s_GLFWInitialized)
		{
			int success = glfwInit();
			ATN_CORE_ASSERT(success, "Could not intialize GLFW");
			glfwSetErrorCallback(GLFWErrorCallback);
			s_GLFWInitialized = true;
		}
			
		m_Window = glfwCreateWindow((int)m_Desc.Width, 
									(int)m_Desc.Height, 
									m_Desc.Title.c_str(), 
									nullptr, 
									nullptr);

		glfwMakeContextCurrent(m_Window);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		ATN_CORE_ASSERT(status, "Failed to initialize Glad");
		glfwSetWindowUserPointer(m_Window, &m_Desc);
		if (m_Desc.VSync)
			glfwSwapInterval(1);	// VSync is disabled by default

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowDesc& data = *reinterpret_cast<WindowDesc*>(glfwGetWindowUserPointer(window));
				data.Width = width;
				data.Height = height;

				WindowResizedEvent event(width, height);	
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow * window)
			{
				WindowDesc& data = *reinterpret_cast<WindowDesc*>(glfwGetWindowUserPointer(window));

				WindowCloseEvent event;
				data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowDesc& data = *reinterpret_cast<WindowDesc*>(glfwGetWindowUserPointer(window));
				
				switch (action)
				{
					case GLFW_PRESS:
					{
						KeyPressedEvent event(key, 0);
						data.EventCallback(event);
						break;
					}
					case GLFW_RELEASE:
					{
						KeyReleasedEvent event(key);
						data.EventCallback(event);
						break;
					}
					case GLFW_REPEAT:
					{
						KeyPressedEvent event(key, 1);
						data.EventCallback(event);
						break;
					}
				}
			});
		
		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int character) 
			{
				WindowDesc& data = *reinterpret_cast<WindowDesc*>(glfwGetWindowUserPointer(window));

				KeyTypedEvent event(character);
				data.EventCallback(event);
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int modes)
			{
				WindowDesc& data = *reinterpret_cast<WindowDesc*>(glfwGetWindowUserPointer(window));

				switch (action)
				{
					case GLFW_PRESS:
					{
						MouseButtonPressedEvent event(button);
						data.EventCallback(event);
						break;
					}
					case GLFW_RELEASE:
					{
						MouseButtonReleasedEvent event(button);
						data.EventCallback(event);
						break;
					}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowDesc& data = *reinterpret_cast<WindowDesc*>(glfwGetWindowUserPointer(window));

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y) 
			{
				WindowDesc& data = *reinterpret_cast<WindowDesc*>(glfwGetWindowUserPointer(window));

				MouseMovedEvent event((float)x, (float)y);
				data.EventCallback(event);
			});

	}


	void WindowsWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
	}


	void WindowsWindow::OnUpdate()
	{
		glfwPollEvents();
		glfwSwapBuffers(m_Window);
	}


	void WindowsWindow::SetVSync(bool enabled)
	{
		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Desc.VSync = enabled;
	}
}
