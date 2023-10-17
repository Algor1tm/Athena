#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/CommandBuffer.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanCommandBuffer: public CommandBuffer
	{
	public:
		VulkanCommandBuffer(CommandBufferUsage usage);
		~VulkanCommandBuffer();

		virtual void Begin() override;
		virtual void End() override;

		virtual void Flush() override;

		VkCommandBuffer GetVulkanCommandBuffer();

	private:
		void FlushForPresent();
		void FlushImmediate();

	private:
		CommandBufferUsage m_Usage;
		std::vector<VkCommandBuffer> m_VkCommandBuffer;
	};
}
