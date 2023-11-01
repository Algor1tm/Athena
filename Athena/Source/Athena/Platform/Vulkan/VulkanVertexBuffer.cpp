#include "VulkanVertexBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Core/Buffer.h"


namespace Athena
{
	VulkanVertexBuffer::VulkanVertexBuffer(const VertexBufferCreateInfo& info)
	{
		m_Info = info;

		Buffer vertexLocalData = Buffer::Copy(info.VerticesData, info.VerticesSize);
		Buffer indexLocalData = Buffer::Copy(info.IndicesData, info.IndicesCount * sizeof(uint32));

		Renderer::Submit([this, vertexLocalData, indexLocalData]() mutable
		{
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;

			VkDevice logicalDevice = VulkanContext::GetLogicalDevice();

			// Vertex buffer
			{
				VkDeviceSize bufferSize = m_Info.VerticesSize;

				VulkanUtils::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

				void* data;
				vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
				memcpy(data, vertexLocalData.Data(), bufferSize);
				vkUnmapMemory(logicalDevice, stagingBufferMemory);

				VulkanUtils::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_VertexBuffer, &m_VertexBufferMemory);

				VulkanUtils::CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

				vkDestroyBuffer(logicalDevice, stagingBuffer, VulkanContext::GetAllocator());
				vkFreeMemory(logicalDevice, stagingBufferMemory, VulkanContext::GetAllocator());
			}

			// Index buffer
			{
				VkDeviceSize bufferSize = m_Info.IndicesCount * sizeof(uint32);

				VulkanUtils::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

				void* data;
				vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
				memcpy(data, indexLocalData.Data(), bufferSize);
				vkUnmapMemory(logicalDevice, stagingBufferMemory);

				VulkanUtils::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_IndexBuffer, &m_IndexBufferMemory);

				VulkanUtils::CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

				vkDestroyBuffer(logicalDevice, stagingBuffer, VulkanContext::GetAllocator());
				vkFreeMemory(logicalDevice, stagingBufferMemory, VulkanContext::GetAllocator());

				vertexLocalData.Release();
				indexLocalData.Release();

				m_Info.IndicesData = nullptr;
				m_Info.VerticesData = nullptr;
			}
		});
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		Renderer::SubmitResourceFree([vertexBuffer = m_VertexBuffer, vertexMemory = m_VertexBufferMemory, 
			indexBuffer = m_IndexBuffer, indexMemory = m_IndexBufferMemory]() 
		{
			vkDestroyBuffer(VulkanContext::GetLogicalDevice(), vertexBuffer, VulkanContext::GetAllocator());
			vkFreeMemory(VulkanContext::GetLogicalDevice(), vertexMemory, VulkanContext::GetAllocator());

			vkDestroyBuffer(VulkanContext::GetLogicalDevice(), indexBuffer, VulkanContext::GetAllocator());
			vkFreeMemory(VulkanContext::GetLogicalDevice(), indexMemory, VulkanContext::GetAllocator());
		});
	}
}
