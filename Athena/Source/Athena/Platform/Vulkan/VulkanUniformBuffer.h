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
		VulkanUniformBuffer(const String& name, uint64 size);
		~VulkanUniformBuffer();

		virtual void UploadData(const void* data, uint64 size, uint64 offset) override;
		virtual uint64 GetSize() override { return m_Size; }

		VkBuffer GetVulkanBuffer(uint32 frameIndex) { return m_VulkanUBSet[frameIndex].GetBuffer(); }
		const VkDescriptorBufferInfo& GetVulkanDescriptorInfo(uint32 frameIndex) const { return m_DescriptorInfo[frameIndex]; }

	private:
		String m_Name;
		std::vector<VulkanBufferAllocation> m_VulkanUBSet;
		std::vector<VkDescriptorBufferInfo> m_DescriptorInfo;
		uint64 m_Size;
	};
}
