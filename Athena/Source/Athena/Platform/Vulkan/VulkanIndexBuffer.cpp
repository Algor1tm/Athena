#include "VulkanIndexBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Core/Buffer.h"


namespace Athena
{
	VulkanIndexBuffer::VulkanIndexBuffer(const IndexBufferCreateInfo& info)
	{
		m_Info = info;

		Buffer indexLocalData = Buffer::Copy(m_Info.Data, info.Data == nullptr ? 0 : m_Info.Count * sizeof(uint32));

		Renderer::Submit([this, indexLocalData]() mutable
		{
			Ref<VulkanAllocator> allocator = VulkanContext::GetAllocator();

			if (m_Info.Usage == BufferUsage::STATIC)
			{
				VkDeviceSize bufferSize = m_Info.Count * sizeof(uint32);

				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

				VulkanBufferAllocation stagingBuffer = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

				void* mappedData = stagingBuffer.MapMemory();
				memcpy(mappedData, indexLocalData.Data(), bufferSize);
				stagingBuffer.UnmapMemory();

				bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				m_IndexBuffer = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, (VmaAllocationCreateFlagBits)0, m_Info.Name);
				Vulkan::SetObjectDebugName(m_IndexBuffer.GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, m_Info.Name);

				Vulkan::CopyBuffer(stagingBuffer.GetBuffer(), m_IndexBuffer.GetBuffer(), bufferSize);

				allocator->DestroyBuffer(stagingBuffer);
			}
			else if (m_Info.Usage == BufferUsage::DYNAMIC)
			{
				VkDeviceSize bufferSize = m_Info.Count * sizeof(uint32);

				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

				m_IndexBuffer = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, m_Info.Name);
				Vulkan::SetObjectDebugName(m_IndexBuffer.GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, m_Info.Name);

				if (m_Info.Data != nullptr)
				{
					void* mappedData = m_IndexBuffer.MapMemory();
					memcpy(mappedData, indexLocalData.Data(), bufferSize);
					m_IndexBuffer.UnmapMemory();
				}
			}

			indexLocalData.Release();
			m_Info.Data = nullptr;
		});
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		Renderer::SubmitResourceFree([indexBuffer = m_IndexBuffer, name = m_Info.Name]()
		{
			VulkanContext::GetAllocator()->DestroyBuffer(indexBuffer, name);
		});
	}

	void VulkanIndexBuffer::RT_SetData(const void* data, uint64 size, uint64 offset)
	{
		ATN_CORE_VERIFY(m_Info.Usage == BufferUsage::DYNAMIC);

		void* mappedData = m_IndexBuffer.MapMemory();
		memcpy((byte*)mappedData + offset, data, size);
		m_IndexBuffer.UnmapMemory();
	}
}
