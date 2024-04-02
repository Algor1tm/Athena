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
		virtual void Resize(uint64 size) override;

		VkBuffer GetVulkanVertexBuffer() const;

	private:
		void CleanUp();

	private:
		VulkanBufferAllocation m_VertexBuffer;
		std::vector<VulkanBufferAllocation> m_VertexBufferSet;
	};
}
