#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/CommandList.h"
#include "Athena/Renderer/RHI/Vulkan/CommandBufferManager.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanCommandList : public CommandList
	{
	public:
		VulkanCommandList();
		virtual ~VulkanCommandList();

		virtual void Submit(bool wait) override;

		VulkanCommandBuffer* GetCommandBuffer();

	private:
		VulkanCommandBuffer* m_CommandBuffer;
	};
}
