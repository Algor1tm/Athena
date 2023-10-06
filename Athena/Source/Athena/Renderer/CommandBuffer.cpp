#include "CommandBuffer.h"

#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanCommandBuffer.h"


namespace Athena
{
	Ref<CommandBuffer> CommandBuffer::Create()
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanCommandBuffer>::Create();
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
