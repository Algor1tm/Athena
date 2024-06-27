#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/GPUBuffer.h"

#include "Athena/Platform/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanStorageBuffer: public StorageBuffer
	{
	public:
		VulkanStorageBuffer(const String& name, uint64 size, BufferMemoryFlags flags);
		~VulkanStorageBuffer();

		virtual void UploadData(const void* data, uint64 size, uint64 offset) override;
		virtual void Resize(uint64 size) override;

		VkBuffer GetVulkanBuffer(uint32 frameIndex) { return m_VulkanSBSet[frameIndex].GetBuffer(); }
		const VkDescriptorBufferInfo& GetVulkanDescriptorInfo(uint32 frameIndex) const { return m_DescriptorInfo[frameIndex]; }

	private:
		void CleanUp();

	private:
		std::vector<VulkanBufferAllocation> m_VulkanSBSet;
		std::vector<VkDescriptorBufferInfo> m_DescriptorInfo;
		BufferMemoryFlags m_Flags;
	};
}
