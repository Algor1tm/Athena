#include "RenderCommandBuffer.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"


namespace Athena
{
	Ref<RenderCommandBuffer> RenderCommandBuffer::Create(const RenderCommandBufferCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanRenderCommandBuffer>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
