#include "atnpch.h"
#include "Athena/Log.h"
#include "WindowsWindow.h"


namespace Athena
{
	static bool s_GLFWInitialized = false;

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

			s_GLFWInitialized = true;
		}

		m_Window = glfwCreateWindow((int)m_Desc.Width, 
									(int)m_Desc.Height, 
									m_Desc.Title.c_str(), 
									nullptr, 
									nullptr);

		glfwMakeContextCurrent(m_Window);
		glfwSetWindowUserPointer(m_Window, &m_Desc);
		if (m_Desc.VSync)
			glfwSwapInterval(1);	// VSync is disabled by default
		SetEventCallback(m_Desc.EventCallback);
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
