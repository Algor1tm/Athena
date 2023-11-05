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

		virtual void SetVertexData(const void* data, uint32 size) override {}

		VkBuffer GetVulkanVertexBuffer() { return m_VertexBuffer.GetBuffer(); }
		VkBuffer GetVulkanIndexBuffer() { return m_IndexBuffer.GetBuffer(); }

	private:
		VulkanBuffer m_VertexBuffer;
		VulkanBuffer m_IndexBuffer;
	};
}
