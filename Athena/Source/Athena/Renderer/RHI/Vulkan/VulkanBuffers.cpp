#include "VulkanBuffers.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"


namespace Athena
{
	namespace Vulkan
	{
		static VkBufferUsageFlags TranslateBufferUsage(GPUBufferType type)
		{
			switch (type)
			{
			case GPUBufferType::VertexBuffer: return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			case GPUBufferType::IndexBuffer: return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			case GPUBufferType::StructuredBuffer: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			}

			checkf(false);
			return 0;
		}
	}

	VulkanGPUBuffer::VulkanGPUBuffer(const GPUBufferCreateInfo& info)
		: GPUBuffer(info)
	{

	}

	VulkanGPUBuffer::~VulkanGPUBuffer()
	{
		if (m_Buffer)
		{
			VulkanRHI::Get()->DeleteFrameResourcesDeferred([buffer = m_Buffer, name = GetName()]()
				{
					VulkanRHI::Get()->GetAllocator()->DestroyBuffer(buffer, name);
				});
		}
	}

	void VulkanGPUBuffer::Init()
	{
		TRACY_PROFILE_FUNC();

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = GetSize();
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		bufferInfo.usage |= Vulkan::TranslateBufferUsage(GetType());

		VK_CHECK(VulkanRHI::Get()->GetAllocator()->AllocateBuffer(&m_Buffer, bufferInfo, VMA_MEMORY_USAGE_AUTO, 0, GetName()));
		VK_SET_OBJECT_NAME(m_Buffer.GetBuffer(), VK_OBJECT_TYPE_BUFFER, GetName());
	}
}
