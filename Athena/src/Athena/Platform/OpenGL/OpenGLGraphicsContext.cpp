#include "atnpch.h"
#include "OpenGLGraphicsContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>


namespace Athena
{
	OpenGLGraphicsContext::OpenGLGraphicsContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		ATN_CORE_ASSERT(windowHandle, "Window handle is null!");
	}

	void OpenGLGraphicsContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		ATN_CORE_ASSERT(status, "Failed to initialize Glad!");

		ATN_CORE_INFO("OpenGL version: {0}", glGetString(GL_VERSION));
		ATN_CORE_INFO("Graphics Card: {0}", glGetString(GL_RENDERER));
		ATN_CORE_INFO("");
	}

	void OpenGLGraphicsContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}
}
