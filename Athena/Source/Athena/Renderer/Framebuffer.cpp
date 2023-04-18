#include "Framebuffer.h"

#include "Athena/Platform/OpenGL/GLFramebuffer.h"

#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	Ref<Framebuffer> Framebuffer::Create(const FramebufferDescription& desc)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::OpenGL:
			return CreateRef<GLFramebuffer>(desc); break;
		case Renderer::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
