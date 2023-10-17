#include "CommandBuffer.h"

#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanCommandBuffer.h"


namespace Athena
{
	Ref<CommandBuffer> CommandBuffer::Create(CommandBufferUsage usage)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanCommandBuffer>::Create(usage);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
