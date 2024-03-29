#include "VulkanUniformBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanUniformBuffer::VulkanUniformBuffer(const String& name, uint64 size)
	{
		m_Size = size;
		m_Name = name;
		m_VulkanUBSet.resize(Renderer::GetFramesInFlight());
		m_DescriptorInfo.resize(Renderer::GetFramesInFlight());

		for (uint32 i = 0; i < m_VulkanUBSet.size(); ++i)
		{
			String bufferName = std::format("{}_{}", m_Name, i);

			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = m_Size;
			bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			m_VulkanUBSet[i] = VulkanContext::GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, bufferName);
			Vulkan::SetObjectDebugName(m_VulkanUBSet[i].GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, bufferName);

			VkDescriptorBufferInfo descriptorInfo;
			descriptorInfo.buffer = m_VulkanUBSet[i].GetBuffer();
			descriptorInfo.offset = 0;
			descriptorInfo.range = m_Size;
			m_DescriptorInfo[i] = descriptorInfo;
		}
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		Renderer::SubmitResourceFree([vulkanUBSet = m_VulkanUBSet, name = m_Name]()
		{
			for (uint32 i = 0; i < vulkanUBSet.size(); ++i)
			{
				VulkanContext::GetAllocator()->DestroyBuffer(vulkanUBSet[i], std::format("{}_{}", name, i));
			}
		});
	}

	void VulkanUniformBuffer::UploadData(const void* data, uint64 size, uint64 offset)
	{
		auto& ubo = m_VulkanUBSet[Renderer::GetCurrentFrameIndex()];

		void* mappedMemory = ubo.MapMemory();
		memcpy((byte*)mappedMemory + offset, data, size);
		ubo.UnmapMemory();
	}
}
