#include "atnpch.h"
#include "Framebuffer.h"

#include "Renderer.h"
#include "Athena/Platform/OpenGL/OpenGLFramebuffer.h"


namespace Athena
{
	Ref<Framebuffer> Framebuffer::Create(const FramebufferDESC& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLFramebuffer>(desc); break;
		case RendererAPI::API::Direct3D:
			ATN_CORE_ASSERT(false, "Renderer API Direct3D is not supported"); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
