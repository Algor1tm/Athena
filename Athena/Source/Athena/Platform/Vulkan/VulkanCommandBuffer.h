#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/CommandBuffer.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanCommandBuffer: public CommandBuffer
	{
	public:
		VulkanCommandBuffer();
		~VulkanCommandBuffer();

		virtual void Begin() override;
		virtual void End() override;

		// Temporary function
		virtual void* GetCommandBuffer() override { return (void*)GetNativeCmdBuffer(); }

		VkCommandBuffer GetNativeCmdBuffer();

	private:
		std::vector<VkCommandBuffer> m_CommandBuffers;
	};
}
