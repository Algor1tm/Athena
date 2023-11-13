#include "Framebuffer.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanFramebuffer.h"


namespace Athena
{
	Ref<Framebuffer> Framebuffer::Create(const FramebufferCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanFramebuffer>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
