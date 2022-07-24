#pragma once

#include "Athena/Renderer/GraphicsContext.h"


struct GLFWwindow;


namespace Athena
{
	class ATHENA_API OpenGLGraphicsContext : public GraphicsContext
	{
	public:
		OpenGLGraphicsContext(GLFWwindow* windowHandle);

		void Init() override;
		void SwapBuffers() override;

	private:
		GLFWwindow* m_WindowHandle;
	};
}

