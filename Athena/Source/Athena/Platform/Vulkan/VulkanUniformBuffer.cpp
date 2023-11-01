#include "VulkanUniformBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanUniformBuffer::VulkanUniformBuffer(uint32 size)
	{
		m_VulkanUBOSet.resize(Renderer::GetFramesInFlight());

		Renderer::Submit([this, size]()
		{
			for (auto& ubo : m_VulkanUBOSet)
			{
				VulkanUtils::CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ubo.Buffer, &ubo.Memory);
			}
		});

	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		Renderer::SubmitResourceFree([vulkanUBOset = m_VulkanUBOSet]()
		{
			for (uint32 i = 0; i < vulkanUBOset.size(); ++i)
			{
				vkDestroyBuffer(VulkanContext::GetLogicalDevice(), vulkanUBOset[i].Buffer, VulkanContext::GetAllocator());
				vkFreeMemory(VulkanContext::GetLogicalDevice(), vulkanUBOset[i].Memory, VulkanContext::GetAllocator());
			}
		});
	}

	void VulkanUniformBuffer::SetData(const void* data, uint32 size, uint32 offset)
	{
		auto& ubo = m_VulkanUBOSet[Renderer::GetCurrentFrameIndex()];

		void* mappedMemory;
		vkMapMemory(VulkanContext::GetDevice()->GetLogicalDevice(), ubo.Memory, offset, size, 0, &mappedMemory);
		memcpy(mappedMemory, data, size);
		vkUnmapMemory(VulkanContext::GetDevice()->GetLogicalDevice(), ubo.Memory);
	}
}
