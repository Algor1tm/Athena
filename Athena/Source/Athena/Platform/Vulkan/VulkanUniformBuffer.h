#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/GPUBuffers.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanUniformBuffer: public UniformBuffer
	{
	public:
		VulkanUniformBuffer(uint32 size);
		~VulkanUniformBuffer();

		virtual void SetData(const void* data, uint32 size, uint32 offset) override;

		VkBuffer GetVulkanBuffer(uint32 frameIndex) { return m_VulkanUBOSet[frameIndex].Buffer; }

	private:
		struct VulkanBufferData
		{
			VkBuffer Buffer = VK_NULL_HANDLE;
			VkDeviceMemory Memory = VK_NULL_HANDLE;
		};

	private:
		std::vector<VulkanBufferData> m_VulkanUBOSet;
	};
}
