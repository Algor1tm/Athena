#include "VulkanVertexBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Core/Buffer.h"


namespace Athena
{
	VulkanVertexBuffer::VulkanVertexBuffer(const VertexBufferCreateInfo& info)
	{
		m_Info = info;

		m_Info.Size = 0;
		Resize(info.Size);
	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		CleanUp();
	}

	void VulkanVertexBuffer::CleanUp()
	{
		Renderer::SubmitResourceFree([vertexBuffer = m_VertexBuffer, vertexBufferSet = m_VertexBufferSet, name = m_Info.Name]()
		{
			if (vertexBuffer)
				VulkanContext::GetAllocator()->DestroyBuffer(vertexBuffer, name);

			for (uint32 i = 0; i < vertexBufferSet.size(); ++i)
				VulkanContext::GetAllocator()->DestroyBuffer(vertexBufferSet[i], std::format("{}_{}", name, i));
		});

		m_VertexBufferSet.clear();
	}

	void VulkanVertexBuffer::UploadData(const void* data, uint64 size, uint64 offset)
	{
		ATN_CORE_ASSERT(m_Info.Flags == BufferMemoryFlags::CPU_WRITEABLE);

		if (size == 0)
			return;

		auto buffer = m_VertexBufferSet[Renderer::GetCurrentFrameIndex()];

		void* mappedData = buffer.MapMemory();
		memcpy((byte*)mappedData + offset, data, size);
		buffer.UnmapMemory();
	}

	void VulkanVertexBuffer::Resize(uint64 size)
	{
		if (m_Info.Size == size)
			return;

		CleanUp();

		m_Info.Size = size;

		Ref<VulkanAllocator> allocator = VulkanContext::GetAllocator();

		if (m_Info.Flags == BufferMemoryFlags::GPU_ONLY)
		{
			VkDeviceSize bufferSize = m_Info.Size;

			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = bufferSize;
			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

			VulkanBufferAllocation stagingBuffer = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

			void* mappedData = stagingBuffer.MapMemory();
			memcpy(mappedData, m_Info.Data, bufferSize);
			stagingBuffer.UnmapMemory();

			bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			m_VertexBuffer = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, (VmaAllocationCreateFlagBits)0, m_Info.Name);
			Vulkan::SetObjectDebugName(m_VertexBuffer.GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, m_Info.Name);

			Vulkan::CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer.GetBuffer(), bufferSize);

			allocator->DestroyBuffer(stagingBuffer);
		}
		else if (m_Info.Flags == BufferMemoryFlags::CPU_WRITEABLE)
		{
			m_VertexBufferSet.resize(Renderer::GetFramesInFlight());

			for (uint32 i = 0; i < m_VertexBufferSet.size(); ++i)
			{
				String bufferName = std::format("{}_{}", m_Info.Name, i);
				VkDeviceSize bufferSize = m_Info.Size;

				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = bufferSize;
				bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

				m_VertexBufferSet[i] = allocator->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, bufferName);
				Vulkan::SetObjectDebugName(m_VertexBufferSet[i].GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, bufferName);

				if (m_Info.Data != nullptr)
				{
					void* mappedData = m_VertexBufferSet[i].MapMemory();
					memcpy(mappedData, m_Info.Data, bufferSize);
					m_VertexBufferSet[i].UnmapMemory();
				}
			}
		}

		m_Info.Data = nullptr;
	}

	VkBuffer VulkanVertexBuffer::GetVulkanVertexBuffer() const
	{
		if (m_Info.Flags == BufferMemoryFlags::GPU_ONLY)
			return m_VertexBuffer.GetBuffer();

		return m_VertexBufferSet[Renderer::GetCurrentFrameIndex()].GetBuffer();
	}
}
