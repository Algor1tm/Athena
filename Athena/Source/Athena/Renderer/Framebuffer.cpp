#include "atnpch.h"
#include "Framebuffer.h"

#include "Renderer.h"
#include "Athena/Platform/OpenGL/GLFramebuffer.h"
#include "Athena/Platform/Direct3D/D3D11Framebuffer.h"


namespace Athena
{
	Ref<Framebuffer> Framebuffer::Create(const FramebufferDescription& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLFramebuffer>(desc); break;
		case RendererAPI::API::Direct3D:
			return CreateRef<D3D11Framebuffer>(desc); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
