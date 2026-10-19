#include "VulkanBuffers.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanVertexBuffer::VulkanVertexBuffer(const String& name, uint64 size)
		: VertexBuffer(name, size)
	{

	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		CleanUp();
	}

	void VulkanVertexBuffer::Init()
	{
		TRACY_PROFILE_FUNC();

		Resize(GetSize());
	}

	void VulkanVertexBuffer::Upload(const Buffer& buffer, uint64 offset)
	{
		TRACY_PROFILE_FUNC();

		check(GetSize() >= buffer.Size() +offset  && buffer.Data() != nullptr, "Invalid initialization buffer for VertexBuffer!");

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = buffer.Size();
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VulkanBufferAllocation stagingBuffer = VulkanRHI::Get()->GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		void* mappedData = stagingBuffer.MapMemory();
		memcpy(mappedData, buffer.Data(), buffer.Size());
		stagingBuffer.UnmapMemory();

		VulkanCommandBuffer* uploadCmdBuffer = VulkanRHI::Get()->GetCmdBufferManager()->GetTransferCommandBuffer();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = offset;
		copyRegion.size = buffer.Size();

		vkCmdCopyBuffer(uploadCmdBuffer->CommandBuffer, stagingBuffer.GetBuffer(), m_VertexBuffer.GetBuffer(), 1, &copyRegion);

		VkBufferMemoryBarrier bufferBarrier{};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.buffer = m_VertexBuffer.GetBuffer();
		bufferBarrier.offset = offset;
		bufferBarrier.size = buffer.Size();

		vkCmdPipelineBarrier(
			uploadCmdBuffer->CommandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, 
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			0, 
			0, nullptr,
			1, &bufferBarrier,
			0, nullptr
		);

		uploadCmdBuffer->AddCallbackOnCompletion([stagingBuffer]()
			{
				VulkanRHI::Get()->GetAllocator()->DestroyBuffer(stagingBuffer);
			});
	}

	void VulkanVertexBuffer::Resize(uint64 size)
	{
		TRACY_PROFILE_FUNC();

		VertexBuffer::Resize(size);

		CleanUp();

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = GetSize();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		m_VertexBuffer = VulkanRHI::Get()->GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, 0, GetName());
		VK_SET_OBJECT_NAME(m_VertexBuffer.GetBuffer(), VK_OBJECT_TYPE_BUFFER, GetName());
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


	VulkanIndexBuffer::VulkanIndexBuffer(const String& name, uint64 size, IndexSize indexSize)
		: IndexBuffer(name, size, indexSize)
	{

	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		CleanUp();
	}

	void VulkanIndexBuffer::Init()
	{
		TRACY_PROFILE_FUNC();

		Resize(GetSize());
	}

	void VulkanIndexBuffer::Upload(const Buffer& buffer, uint64 offset)
	{
		TRACY_PROFILE_FUNC();

		check(GetSize() >= buffer.Size() + offset && buffer.Data() != nullptr, "Invalid initialization buffer for VertexBuffer!");

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = GetSize();
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VulkanBufferAllocation stagingBuffer = VulkanRHI::Get()->GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		void* mappedData = stagingBuffer.MapMemory();
		memcpy(mappedData, buffer.Data(), buffer.Size());
		stagingBuffer.UnmapMemory();

		VulkanCommandBuffer* uploadCmdBuffer = VulkanRHI::Get()->GetCmdBufferManager()->GetTransferCommandBuffer();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = offset;
		copyRegion.size = buffer.Size();

		vkCmdCopyBuffer(uploadCmdBuffer->CommandBuffer, stagingBuffer.GetBuffer(), m_IndexBuffer.GetBuffer(), 1, &copyRegion);

		VkBufferMemoryBarrier bufferBarrier{};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
		bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.buffer = m_IndexBuffer.GetBuffer();
		bufferBarrier.offset = offset;
		bufferBarrier.size = buffer.Size();

		vkCmdPipelineBarrier(
			uploadCmdBuffer->CommandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			0,
			0, nullptr,
			1, &bufferBarrier,
			0, nullptr
		);

		uploadCmdBuffer->AddCallbackOnCompletion([stagingBuffer]() 
			{
				VulkanRHI::Get()->GetAllocator()->DestroyBuffer(stagingBuffer);
			});
	}

	void VulkanIndexBuffer::Resize(uint64 size)
	{
		TRACY_PROFILE_FUNC();

		IndexBuffer::Resize(size);

		CleanUp();

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = GetSize();
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		m_IndexBuffer = VulkanRHI::Get()->GetAllocator()->AllocateBuffer(bufferInfo, VMA_MEMORY_USAGE_AUTO, 0, GetName());
		VK_SET_OBJECT_NAME(m_IndexBuffer.GetBuffer(), VK_OBJECT_TYPE_BUFFER, GetName());
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
