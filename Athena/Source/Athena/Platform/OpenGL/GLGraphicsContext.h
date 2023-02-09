#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/GraphicsContext.h"


struct GLFWwindow;

namespace Athena
{
	class ATHENA_API GLGraphicsContext : public GraphicsContext
	{
	public:
		GLGraphicsContext(GLFWwindow* windowHandle);

		virtual void SwapBuffers() override;
		virtual void SetVSync(bool enabled) override;
		virtual void SetFullscreen(bool enabled) override {};

	private:
		GLFWwindow* m_WindowHandle;
	};
}
