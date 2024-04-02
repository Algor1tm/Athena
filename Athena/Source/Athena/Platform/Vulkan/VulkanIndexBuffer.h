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

		virtual void UploadData(const void* data, uint64 size, uint64 offset = 0) override;
		virtual void Resize(uint64 size) override;

		VkBuffer GetVulkanIndexBuffer() const;

	private:
		void CleanUp();

	private:
		VulkanBufferAllocation m_IndexBuffer;
		std::vector<VulkanBufferAllocation> m_IndexBufferSet;
	};
}
