#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"


namespace Athena
{
	class ATHENA_API VulkanGPUBuffer : public GPUBuffer
	{
	public:
		VulkanGPUBuffer(const GPUBufferCreateInfo& info);
		virtual ~VulkanGPUBuffer();

		virtual void Init() override;
		VkBuffer GetVulkanBuffer() const { return m_Buffer.GetBuffer(); }

	private:
		VulkanBufferAllocation m_Buffer;
	};
}
