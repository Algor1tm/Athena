#include "VulkanBuffers.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"


namespace Athena
{
	void VulkanVertexBuffer::Init()
	{
		Resize(GetSize());
	}

	void VulkanVertexBuffer::Destroy()
	{
		CleanUp();
	}

	void VulkanVertexBuffer::UploadData(const Buffer& buffer, uint64 offset)
	{
		check(GetSize() >= buffer.Size() && buffer.Data() != nullptr, "Invalid initialization buffer for VertexBuffer!");

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = GetSize();
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VulkanBufferAllocation stagingBuffer = VulkanRHI::Get()->GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		void* mappedData = stagingBuffer.MapMemory();
		memcpy(mappedData, buffer.Data(), buffer.Size());
		stagingBuffer.UnmapMemory();

		VulkanCommandBuffer* uploadCmdBuffer = VulkanRHI::Get()->GetCmdBufferManager()->GetUploadCommandBuffer();
		Vulkan::CopyBuffer(uploadCmdBuffer, stagingBuffer.GetBuffer(), m_VertexBuffer.GetBuffer(), buffer.Size());

		uploadCmdBuffer->AddCallbackOnCompletion([stagingBuffer]()
			{
				VulkanRHI::Get()->GetAllocator()->DestroyBuffer(stagingBuffer);
			});
	}

	void VulkanVertexBuffer::Resize(uint64 size)
	{
		VertexBuffer::Resize(size);

		CleanUp();

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = GetSize();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		m_VertexBuffer = VulkanRHI::Get()->GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, 0, GetName());
		VULKAN_SET_DEBUG_NAME(m_VertexBuffer.GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, GetName());
	}

	void VulkanVertexBuffer::CleanUp()
	{
		if (m_VertexBuffer)
		{
			VulkanRHI::Get()->DeleteFrameResourcesDeferred([vertexBuffer = m_VertexBuffer, name = GetName()]()
				{
					VulkanRHI::Get()->GetAllocator()->DestroyBuffer(vertexBuffer, name);
				});
		}
	}


	void VulkanIndexBuffer::Init()
	{
		Resize(GetSize());
	}

	void VulkanIndexBuffer::Destroy()
	{
		CleanUp();
	}

	void VulkanIndexBuffer::UploadData(const Buffer& buffer, uint64 offset)
	{
		check(GetSize() >= buffer.Size() && buffer.Data() != nullptr, "Invalid initialization buffer for VertexBuffer!");

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = GetSize();
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VulkanBufferAllocation stagingBuffer = VulkanRHI::Get()->GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		void* mappedData = stagingBuffer.MapMemory();
		memcpy(mappedData, buffer.Data(), buffer.Size());
		stagingBuffer.UnmapMemory();

		VulkanCommandBuffer* uploadCmdBuffer = VulkanRHI::Get()->GetCmdBufferManager()->GetUploadCommandBuffer();
		Vulkan::CopyBuffer(uploadCmdBuffer, stagingBuffer.GetBuffer(), m_IndexBuffer.GetBuffer(), buffer.Size());

		uploadCmdBuffer->AddCallbackOnCompletion([stagingBuffer]() 
			{
				VulkanRHI::Get()->GetAllocator()->DestroyBuffer(stagingBuffer);
			});
	}

	void VulkanIndexBuffer::Resize(uint64 size)
	{
		IndexBuffer::Resize(size);

		CleanUp();

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = GetSize();
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		m_IndexBuffer = VulkanRHI::Get()->GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, 0, GetName());
		VULKAN_SET_DEBUG_NAME(m_IndexBuffer.GetBuffer(), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, GetName());
	}

	void VulkanIndexBuffer::CleanUp()
	{
		if (m_IndexBuffer)
		{
			VulkanRHI::Get()->DeleteFrameResourcesDeferred([vertexBuffer = m_IndexBuffer, name = GetName()]()
				{
					VulkanRHI::Get()->GetAllocator()->DestroyBuffer(vertexBuffer, name);
				});
		}
	}
}
