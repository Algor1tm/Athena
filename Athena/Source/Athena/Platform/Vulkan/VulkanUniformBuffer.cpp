#include "VulkanUniformBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanUniformBuffer::VulkanUniformBuffer(const String& name, uint64 size)
	{
		m_Size = size;
		m_Name = name;
		m_VulkanUBOSet.resize(Renderer::GetFramesInFlight());
		m_DescriptorInfo.resize(Renderer::GetFramesInFlight());

		Renderer::Submit([this]()
		{
			for (uint32 i = 0; i < m_VulkanUBOSet.size(); ++i)
			{
				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = m_Size;
				bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
				m_VulkanUBOSet[i] = VulkanContext::GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, m_Name);

				VkDescriptorBufferInfo descriptorInfo;
				descriptorInfo.buffer = m_VulkanUBOSet[i].GetBuffer();
				descriptorInfo.offset = 0;
				descriptorInfo.range = m_Size;
				m_DescriptorInfo[i] = descriptorInfo;
			}
		});
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		Renderer::SubmitResourceFree([vulkanUBOSet = m_VulkanUBOSet, name = m_Name]()
		{
			for (auto ubo: vulkanUBOSet)
			{
				VulkanContext::GetAllocator()->DestroyBuffer(ubo, name);
			}
		});
	}

	void VulkanUniformBuffer::RT_SetData(const void* data, uint64 size, uint64 offset)
	{
		auto& ubo = m_VulkanUBOSet[Renderer::GetCurrentFrameIndex()];

		void* mappedMemory = ubo.MapMemory();
		memcpy(mappedMemory, data, size);
		ubo.UnmapMemory();
	}
}
