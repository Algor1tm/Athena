#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RenderCommandBuffer.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanRenderCommandBuffer : public RenderCommandBuffer
	{
	public:
		VulkanRenderCommandBuffer(const RenderCommandBufferCreateInfo& info);
		~VulkanRenderCommandBuffer();

		virtual void Begin() override;
		virtual void End() override;
		virtual void Submit() override;

		VkCommandBuffer GetActiveCommandBuffer();

	private:
		void SubmitForPresent();
		void SubmitImmediate();

	private:
		std::vector<VkCommandBuffer> m_CommandBuffers;
	};
}
