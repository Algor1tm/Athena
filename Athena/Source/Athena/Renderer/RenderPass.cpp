#include "RenderPass.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanRenderPass.h"


namespace Athena
{
	Ref<RenderPass> RenderPass::Create(const RenderPassCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanRenderPass>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}