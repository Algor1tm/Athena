#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Platform/Vulkan/VulkanAllocator.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(const VertexBufferCreateInfo& info);
		~VulkanVertexBuffer();

		virtual void UploadData(const void* data, uint64 size, uint64 offset = 0) override;
		VkBuffer GetVulkanVertexBuffer() { return m_VertexBuffer.GetBuffer(); }

	private:
		VulkanBufferAllocation m_VertexBuffer;
	};
}
