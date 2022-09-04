#include "atnpch.h"

#include "WindowsWindow.h"
#include "Athena/Platform/OpenGL/OpenGLGraphicsContext.h"

#include "Athena/Input/Events/ApplicationEvent.h"
#include "Athena/Input/Events/KeyEvent.h"
#include "Athena/Input/Events/MouseEvent.h"

#include <GLFW/glfw3.h>


namespace Athena
{
	static uint8 s_GLFWWindowCount = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		ATN_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	Scope<Window> Window::Create(const WindowDESC& desc)
	{
		return CreateScope<WindowsWindow>(desc);
	}

	WindowsWindow::WindowsWindow(const WindowDESC& desc)
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
		if (s_GLFWWindowCount == 0)
		{
			int success = glfwInit();
			ATN_CORE_ASSERT(success, "Could not intialize GLFW");
			ATN_CORE_INFO("Init GLFW");
			glfwSetErrorCallback(GLFWErrorCallback);
		}
			
		m_Window = glfwCreateWindow((int)m_Desc.Width, 
									(int)m_Desc.Height, 
									m_Desc.Title.c_str(), 
									nullptr, 
									nullptr);
		s_GLFWWindowCount++;

		ATN_CORE_INFO("Create Windows Window '{0}' ({1}, {2})", m_Desc.Title, m_Desc.Width, m_Desc.Height);

		m_Context = GraphicsContext::Create(m_Window);
		m_Context->Init();

		glfwSetWindowUserPointer(m_Window, &m_Desc);
		SetVSync(m_Desc.VSync);

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowDESC& data = *reinterpret_cast<WindowDESC*>(glfwGetWindowUserPointer(window));
				data.Width = width;
				data.Height = height;

				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow * window)
			{
				WindowDESC& data = *reinterpret_cast<WindowDESC*>(glfwGetWindowUserPointer(window));

				WindowCloseEvent event;
				data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowDESC& data = *reinterpret_cast<WindowDESC*>(glfwGetWindowUserPointer(window));
				
				switch (action)
				{
					case GLFW_PRESS:
					{
						KeyPressedEvent event((Keyboard::Key)key, false);
						data.EventCallback(event);
						break;
					}
					case GLFW_RELEASE:
					{
						KeyReleasedEvent event((Keyboard::Key)key);
						data.EventCallback(event);
						break;
					}
					case GLFW_REPEAT:
					{
						KeyPressedEvent event((Keyboard::Key)key, true);
						data.EventCallback(event);
						break;
					}
				}
			});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int character) 
			{
				WindowDESC& data = *reinterpret_cast<WindowDESC*>(glfwGetWindowUserPointer(window));

				KeyTypedEvent event((Keyboard::Key)character);
				data.EventCallback(event);
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int modes)
			{
				WindowDESC& data = *reinterpret_cast<WindowDESC*>(glfwGetWindowUserPointer(window));

				switch (action)
				{
					case GLFW_PRESS:
					{
						MouseButtonPressedEvent event((Mouse::Button)button);
						data.EventCallback(event);
						break;
					}
					case GLFW_RELEASE:
					{
						MouseButtonReleasedEvent event((Mouse::Button)button);
						data.EventCallback(event);
						break;
					}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowDESC& data = *reinterpret_cast<WindowDESC*>(glfwGetWindowUserPointer(window));

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double x, double y) 
			{
				WindowDESC& data = *reinterpret_cast<WindowDESC*>(glfwGetWindowUserPointer(window));

				MouseMovedEvent event((float)x, (float)y);
				data.EventCallback(event);
			});
	}

	void WindowsWindow::Shutdown()
	{
		glfwDestroyWindow(m_Window);
		--s_GLFWWindowCount;

		ATN_CORE_INFO("Shutdown Windows Window '{0}'", m_Desc.Title);

		if (s_GLFWWindowCount == 0)
			glfwTerminate();

		ATN_CORE_INFO("Shutdown GLFW");
	}

	void WindowsWindow::OnUpdate()
	{
		glfwPollEvents();
		m_Context->SwapBuffers();
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