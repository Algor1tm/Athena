#include "VulkanVertexBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Core/Buffer.h"


namespace Athena
{
	VulkanVertexBuffer::VulkanVertexBuffer(const VertexBufferCreateInfo& info)
	{
		m_Info = info;

		Buffer vertexLocalData = Buffer::Copy(m_Info.Data, info.Data == nullptr ? 0 : m_Info.Size);

		Renderer::Submit([this, vertexLocalData]() mutable
		{
			Ref<VulkanAllocator> allocator = VulkanContext::GetAllocator();

			if(m_Info.Usage == BufferUsage::STATIC)
			{
				VkDeviceSize bufferSize = m_Info.Size;

				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

				VulkanBufferAllocation stagingBuffer = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

				void* mappedData = stagingBuffer.MapMemory();
				memcpy(mappedData, vertexLocalData.Data(), bufferSize);
				stagingBuffer.UnmapMemory();

				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				m_VertexBuffer = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, (VmaAllocationCreateFlagBits)0, m_Info.Name);
				Vulkan::SetObjectDebugName(m_VertexBuffer.GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, m_Info.Name);

				Vulkan::CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer.GetBuffer(), bufferSize);

				allocator->DestroyBuffer(stagingBuffer);
			}
			else if (m_Info.Usage == BufferUsage::DYNAMIC)
			{
				VkDeviceSize bufferSize = m_Info.Size;

				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

				m_VertexBuffer = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, m_Info.Name);
				Vulkan::SetObjectDebugName(m_VertexBuffer.GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, m_Info.Name);

				if (m_Info.Data != nullptr)
				{
					void* mappedData = m_VertexBuffer.MapMemory();
					memcpy(mappedData, vertexLocalData.Data(), bufferSize);
					m_VertexBuffer.UnmapMemory();
				}
			}

			vertexLocalData.Release();
			m_Info.Data = nullptr;
		});
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		Renderer::SubmitResourceFree([vertexBuffer = m_VertexBuffer, name = m_Info.Name]() 
		{
			VulkanContext::GetAllocator()->DestroyBuffer(vertexBuffer, name);
		});

		m_Info.IndexBuffer.Release();
	}

	void VulkanVertexBuffer::RT_SetData(const void* data, uint64 size, uint64 offset)
	{
		ATN_CORE_VERIFY(m_Info.Usage == BufferUsage::DYNAMIC);

		void* mappedData = m_VertexBuffer.MapMemory();
		memcpy((byte*)mappedData + offset, data, size);
		m_VertexBuffer.UnmapMemory();
	}
}
