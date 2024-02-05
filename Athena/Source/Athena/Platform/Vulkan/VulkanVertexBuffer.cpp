#include "VulkanVertexBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Core/Buffer.h"


namespace Athena
{
	namespace Vulkan
	{
		static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
		{
			VkCommandBuffer vkCommandBuffer = BeginSingleTimeCommands();
			{
				VkBufferCopy copyRegion{};
				copyRegion.srcOffset = 0;
				copyRegion.dstOffset = 0;
				copyRegion.size = size;

				vkCmdCopyBuffer(vkCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
			}
			EndSingleTimeCommands(vkCommandBuffer);
		}
	}


	VulkanVertexBuffer::VulkanVertexBuffer(const VertexBufferCreateInfo& info)
	{
		m_Info = info;

		Buffer vertexLocalData = Buffer::Copy(info.VerticesData, info.VerticesSize);
		Buffer indexLocalData = Buffer::Copy(info.IndicesData, info.IndicesCount * sizeof(uint32));

		Renderer::Submit([this, vertexLocalData, indexLocalData]() mutable
		{
			Ref<VulkanAllocator> allocator = VulkanContext::GetAllocator();

			// Vertex buffer
			{
				VkDeviceSize bufferSize = m_Info.VerticesSize;

				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

				VulkanBufferAllocation stagingBuffer = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

				void* mappedData = stagingBuffer.MapMemory();
				memcpy(mappedData, vertexLocalData.Data(), bufferSize);
				stagingBuffer.UnmapMemory();

				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				m_VertexBuffer = allocator->AllocateBuffer(bufferInfo);

				Vulkan::CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer.GetBuffer(), bufferSize);

				allocator->DestroyBuffer(stagingBuffer);
				vertexLocalData.Release();
				m_Info.VerticesData = nullptr;
			}

			// Index buffer
			{
				VkDeviceSize bufferSize = m_Info.IndicesCount * sizeof(uint32);

				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

				VulkanBufferAllocation stagingBuffer = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

				void* mappedData = stagingBuffer.MapMemory();
				memcpy(mappedData, indexLocalData.Data(), bufferSize);
				stagingBuffer.UnmapMemory();

				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				m_IndexBuffer = allocator->AllocateBuffer(bufferInfo);

				Vulkan::CopyBuffer(stagingBuffer.GetBuffer(), m_IndexBuffer.GetBuffer(), bufferSize);

				allocator->DestroyBuffer(stagingBuffer);
				indexLocalData.Release();
				m_Info.IndicesData = nullptr;
			}
		});
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		Renderer::SubmitResourceFree([vertexBuffer = m_VertexBuffer, indexBuffer = m_IndexBuffer]() 
		{
			VulkanContext::GetAllocator()->DestroyBuffer(vertexBuffer);
			VulkanContext::GetAllocator()->DestroyBuffer(indexBuffer);
		});
	}
}
