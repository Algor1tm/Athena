#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/GPUBuffers.h"

#include "Athena/Platform/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanUniformBuffer: public UniformBuffer
	{
	public:
		VulkanUniformBuffer(uint32 size);
		~VulkanUniformBuffer();

		virtual void SetData(const void* data, uint32 size, uint32 offset) override;

		VkBuffer GetVulkanBuffer(uint32 frameIndex) { return m_VulkanUBOSet[frameIndex].GetBuffer(); }

	private:
		std::vector<VulkanBuffer> m_VulkanUBOSet;
	};
}
