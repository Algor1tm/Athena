#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/GPUBuffers.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(const VertexBufferCreateInfo& info);
		~VulkanVertexBuffer();

		virtual void SetVertexData(const void* data, uint32 size) override {}

		VkBuffer GetVulkanVertexBuffer() { return m_VertexBuffer; }
		VkBuffer GetVulkanIndexBuffer() { return m_IndexBuffer; }

	private:
		VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_VertexBufferMemory = VK_NULL_HANDLE;
		VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory m_IndexBufferMemory = VK_NULL_HANDLE;
	};
}
