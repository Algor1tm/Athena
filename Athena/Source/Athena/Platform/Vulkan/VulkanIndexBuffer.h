#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Platform/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(const IndexBufferCreateInfo& info);
		~VulkanIndexBuffer();

		virtual void RT_SetData(const void* data, uint64 size, uint64 offset = 0) override;
		VkBuffer GetVulkanIndexBuffer() { return m_IndexBuffer.GetBuffer(); }

	private:
		VulkanBufferAllocation m_IndexBuffer;
	};
}
