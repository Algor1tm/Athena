#include "atnpch.h"
#include "GLGraphicsContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>


namespace Athena
{
	GLGraphicsContext::GLGraphicsContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		ATN_CORE_ASSERT(windowHandle, "Window handle is null!");

		glfwMakeContextCurrent(windowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		ATN_CORE_ASSERT(status, "Failed to initialize glad!");

		ATN_CORE_INFO("");
		ATN_CORE_INFO("Create OpenGL Graphics Context:");
		ATN_CORE_INFO("OpenGL version: {0}", glGetString(GL_VERSION));
		ATN_CORE_INFO("Graphics Card: {0}", glGetString(GL_RENDERER));
		ATN_CORE_INFO("Vendor: {0}", glGetString(GL_VENDOR));
		ATN_CORE_INFO("");
	}

	void GLGraphicsContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

	void GLGraphicsContext::SetVSync(bool enabled)
	{
		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);
	}
}
