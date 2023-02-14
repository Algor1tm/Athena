#include "GraphicsContext.h"

#include "Athena/Platform/OpenGL/GLGraphicsContext.h"

#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	Ref<GraphicsContext> GraphicsContext::Create(void *windowHandle)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLGraphicsContext>(reinterpret_cast<GLFWwindow*>(windowHandle)); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
