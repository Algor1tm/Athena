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

		virtual void BeginDebugLabel(const String& name, const IntegerColor& color = IntegerColor::Black) override;
		virtual void EndDebugLabel() override;
		virtual void InsertDebugLabel(const String& name, const IntegerColor& color = IntegerColor::Black) override;

		VulkanCommandBuffer* GetCommandBuffer();

	private:
		VulkanCommandBuffer* m_CommandBuffer;
	};
}
