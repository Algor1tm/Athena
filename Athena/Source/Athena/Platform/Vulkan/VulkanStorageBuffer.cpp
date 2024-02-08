#include "VulkanStorageBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanStorageBuffer::VulkanStorageBuffer(const String& name, uint64 size)
	{
		m_Size = size;
		m_Name = name;
		m_VulkanSBSet.resize(Renderer::GetFramesInFlight());
		m_DescriptorInfo.resize(Renderer::GetFramesInFlight());

		Renderer::Submit([this]()
		{
			for (uint32 i = 0; i < m_VulkanSBSet.size(); ++i)
			{
				String bufferName = std::format("{}_{}", m_Name, i);

				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = m_Size;
				bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
				m_VulkanSBSet[i] = VulkanContext::GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, bufferName);
				Vulkan::SetObjectDebugName(m_VulkanSBSet[i].GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, bufferName);

				VkDescriptorBufferInfo descriptorInfo;
				descriptorInfo.buffer = m_VulkanSBSet[i].GetBuffer();
				descriptorInfo.offset = 0;
				descriptorInfo.range = m_Size;
				m_DescriptorInfo[i] = descriptorInfo;
			}
		});
	}

	VulkanStorageBuffer::~VulkanStorageBuffer()
	{
		Renderer::SubmitResourceFree([vulkanSBSet = m_VulkanSBSet, name = m_Name]()
		{
			for (uint32 i = 0; i < vulkanSBSet.size(); ++i)
			{
				VulkanContext::GetAllocator()->DestroyBuffer(vulkanSBSet[i], std::format("{}_{}", name, i));
			}
		});
	}

	void VulkanStorageBuffer::RT_SetData(const void* data, uint64 size, uint64 offset)
	{
		auto& ubo = m_VulkanSBSet[Renderer::GetCurrentFrameIndex()];

		void* mappedMemory = ubo.MapMemory();
		memcpy(mappedMemory, data, size);
		ubo.UnmapMemory();
	}
}
