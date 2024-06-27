#include "VulkanIndexBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Core/Buffer.h"


namespace Athena
{
	VulkanIndexBuffer::VulkanIndexBuffer(const IndexBufferCreateInfo& info)
	{
		m_Info = info;

		m_Info.Count = 0;
		Resize(info.Count * sizeof(uint32));
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		CleanUp();
	}

	void VulkanIndexBuffer::CleanUp()
	{
		Renderer::SubmitResourceFree([indexBuffer = m_IndexBuffer, indexBufferSet = m_IndexBufferSet, name = m_Info.Name]()
		{
			if (indexBuffer)
				VulkanContext::GetAllocator()->DestroyBuffer(indexBuffer, name);

			for (uint32 i = 0; i < indexBufferSet.size(); ++i)
				VulkanContext::GetAllocator()->DestroyBuffer(indexBufferSet[i], std::format("{}_{}", name, i));
		});

		m_IndexBufferSet.clear();
	}

	void VulkanIndexBuffer::Resize(uint64 size)
	{
		uint32 count = size / sizeof(uint32);
		if (m_Info.Count == count)
			return;

		CleanUp();

		m_Info.Count = count;

		Ref<VulkanAllocator> allocator = VulkanContext::GetAllocator();

		if (m_Info.Flags == BufferMemoryFlags::GPU_ONLY)
		{
			VkDeviceSize bufferSize = m_Info.Count * sizeof(uint32);

			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = bufferSize;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

			VulkanBufferAllocation stagingBuffer = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

			void* mappedData = stagingBuffer.MapMemory();
			memcpy(mappedData, m_Info.Data, bufferSize);
			stagingBuffer.UnmapMemory();

			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			m_IndexBuffer = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, (VmaAllocationCreateFlagBits)0, m_Info.Name);
			Vulkan::SetObjectDebugName(m_IndexBuffer.GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, m_Info.Name);

			Vulkan::CopyBuffer(stagingBuffer.GetBuffer(), m_IndexBuffer.GetBuffer(), bufferSize);

			allocator->DestroyBuffer(stagingBuffer);
		}
		else if (m_Info.Flags == BufferMemoryFlags::CPU_WRITEABLE)
		{
			m_IndexBufferSet.resize(Renderer::GetFramesInFlight());

			for (uint32 i = 0; i < m_IndexBufferSet.size(); ++i)
			{
				String bufferName = std::format("{}_{}", m_Info.Name, i);
				VkDeviceSize bufferSize = m_Info.Count * sizeof(uint32);

				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

				m_IndexBufferSet[i] = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, bufferName);
				Vulkan::SetObjectDebugName(m_IndexBufferSet[i].GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, bufferName);

				if (m_Info.Data != nullptr)
				{
					void* mappedData = m_IndexBufferSet[i].MapMemory();
					memcpy(mappedData, m_Info.Data, bufferSize);
					m_IndexBufferSet[i].UnmapMemory();
				}
			}
		}

		m_Info.Data = nullptr;
	}

	void VulkanIndexBuffer::UploadData(const void* data, uint64 size, uint64 offset)
	{
		ATN_CORE_VERIFY(m_Info.Flags == BufferMemoryFlags::CPU_WRITEABLE);
			
		auto buffer = m_IndexBufferSet[Renderer::GetCurrentFrameIndex()];

		void* mappedData = buffer.MapMemory();
		memcpy((byte*)mappedData + offset, data, size);
		buffer.UnmapMemory();
	}

	VkBuffer VulkanIndexBuffer::GetVulkanIndexBuffer() const
	{
		if (m_Info.Flags == BufferMemoryFlags::GPU_ONLY)
			return m_IndexBuffer.GetBuffer();

		return m_IndexBufferSet[Renderer::GetCurrentFrameIndex()].GetBuffer();
	}
}
