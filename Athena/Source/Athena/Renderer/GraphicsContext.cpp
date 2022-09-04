#include "atnpch.h"
#include "Athena/Platform/OpenGL/OpenGLGraphicsContext.h"

#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	Ref<GraphicsContext> GraphicsContext::Create(void *windowHandle)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLGraphicsContext>(reinterpret_cast<GLFWwindow*>(windowHandle)); break;
		case RendererAPI::API::Direct3D:
			ATN_CORE_ASSERT(false, "Renderer API Direct3D is not supported"); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
