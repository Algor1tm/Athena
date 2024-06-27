#include "VulkanStorageBuffer.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanStorageBuffer::VulkanStorageBuffer(const String& name, uint64 size, BufferMemoryFlags flags)
	{
		m_Flags = flags;
		m_Name = name;
		m_DescriptorInfo.resize(Renderer::GetFramesInFlight());

		m_Size = 0;
		Resize(size);
	}

	VulkanStorageBuffer::~VulkanStorageBuffer()
	{
		CleanUp();
	}

	void VulkanStorageBuffer::CleanUp()
	{
		Renderer::SubmitResourceFree([vulkanSBSet = m_VulkanSBSet, name = m_Name]()
		{
			for (uint32 i = 0; i < vulkanSBSet.size(); ++i)
			{
				VulkanContext::GetAllocator()->DestroyBuffer(vulkanSBSet[i], std::format("{}_{}", name, i));
			}
		});

		m_VulkanSBSet.clear();
	}

	void VulkanStorageBuffer::UploadData(const void* data, uint64 size, uint64 offset)
	{
		ATN_CORE_ASSERT(m_Flags == BufferMemoryFlags::CPU_WRITEABLE);

		if (size == 0)
			return;

		auto& ubo = m_VulkanSBSet[Renderer::GetCurrentFrameIndex()];

		void* mappedMemory = ubo.MapMemory();
		memcpy(mappedMemory, data, size);
		ubo.UnmapMemory();
	}

	void VulkanStorageBuffer::Resize(uint64 size)
	{
		if (m_Size == size)
			return;

		if(!m_VulkanSBSet.empty())
			CleanUp();

		m_Size = size;

		if (m_Flags == BufferMemoryFlags::GPU_ONLY)
		{
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = m_Size;
			bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

			VulkanBufferAllocation alloc = VulkanContext::GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VmaAllocationCreateFlagBits(0), m_Name);
			Vulkan::SetObjectDebugName(alloc.GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, m_Name);

			VkDescriptorBufferInfo descriptorInfo;
			descriptorInfo.buffer = alloc.GetBuffer();
			descriptorInfo.offset = 0;
			descriptorInfo.range = m_Size;
			m_DescriptorInfo = std::vector<VkDescriptorBufferInfo>(Renderer::GetFramesInFlight(), descriptorInfo);

			m_VulkanSBSet.push_back(alloc);
		}
		else if (m_Flags == BufferMemoryFlags::CPU_WRITEABLE)
		{
			m_VulkanSBSet.resize(Renderer::GetFramesInFlight());

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
		}
	}
}
