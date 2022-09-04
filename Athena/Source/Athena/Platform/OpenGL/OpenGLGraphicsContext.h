#pragma once

#include "Athena/Renderer/GraphicsContext.h"


struct GLFWwindow;


namespace Athena
{
	class ATHENA_API OpenGLGraphicsContext : public GraphicsContext
	{
	public:
		OpenGLGraphicsContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;

	private:
		GLFWwindow* m_WindowHandle;
	};
}

