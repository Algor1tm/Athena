#include "GLGraphicsContext.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>


namespace Athena
{
	GLGraphicsContext::GLGraphicsContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		ATN_CORE_VERIFY(windowHandle, "Window handle is null!");

#ifdef ATN_DEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#endif

		glfwMakeContextCurrent(windowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		ATN_CORE_VERIFY(status, "Failed to initialize glad!");

		ATN_CORE_INFO_TAG("GLGraphicsContext", "Create OpenGL Graphics Context:");
		ATN_CORE_INFO_TAG("GLGraphicsContext", "OpenGL version: {0}", glGetString(GL_VERSION));
		ATN_CORE_INFO_TAG("GLGraphicsContext", "Graphics Card: {0}", glGetString(GL_RENDERER));
		ATN_CORE_INFO_TAG("GLGraphicsContext", "Vendor: {0}", glGetString(GL_VENDOR));
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

	void GLGraphicsContext::GetGPUInfo(GPUInfo* info) const
	{
		info->APIVersion = "OpenGL " + String((char*)glGetString(GL_VERSION));
		info->GPUBrandString = (char*)glGetString(GL_RENDERER);
		info->Vendor = (char*)glGetString(GL_VENDOR);

		GLint total_mem_kb = 0;
		glGetIntegerv(0x9048, &total_mem_kb);

		info->TotalPhysicalMemoryKB = total_mem_kb;
	}
}
