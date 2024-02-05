#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/GPUBuffers.h"

#include "Athena/Platform/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanStorageBuffer: public StorageBuffer
	{
	public:
		VulkanStorageBuffer(const String& name, uint64 size);
		~VulkanStorageBuffer();

		virtual void RT_SetData(const void* data, uint64 size, uint64 offset) override;
		virtual uint64 GetSize() override { return m_Size; }

		VkBuffer GetVulkanBuffer(uint32 frameIndex) { return m_VulkanSBSet[frameIndex].GetBuffer(); }
		const VkDescriptorBufferInfo& GetVulkanDescriptorInfo(uint32 frameIndex) const { return m_DescriptorInfo[frameIndex]; }

	private:
		String m_Name;
		std::vector<VulkanBufferAllocation> m_VulkanSBSet;
		std::vector<VkDescriptorBufferInfo> m_DescriptorInfo;
		uint64 m_Size;
	};
}
