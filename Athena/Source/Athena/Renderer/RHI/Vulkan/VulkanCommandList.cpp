#include "VulkanCommandList.h"

#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanCommandList::VulkanCommandList()
	{
		m_CommandBuffer = VulkanRHI::Get()->GetCmdBufferManager()->GetCommandBuffer();
	}

	VulkanCommandList::~VulkanCommandList()
	{
		check(m_CommandBuffer == nullptr, "Vulkan Command List was not submitted!");

		if (m_CommandBuffer)
		{
			Submit(false);
		}
	}

	void VulkanCommandList::Submit(bool wait)
	{
		VulkanRHI::Get()->GetCmdBufferManager()->Submit(m_CommandBuffer);

		if (wait)
		{
			VulkanRHI::Get()->GetCmdBufferManager()->WaitForCmdBuffer(m_CommandBuffer);
		}

		m_CommandBuffer = nullptr;
	}

	void VulkanCommandList::BeginDebugLabel(const String& name, const IntegerColor& color)
	{
		VK_BEGIN_DEBUG_LABEL(m_CommandBuffer->CommandBuffer, name, color);
	}

	void VulkanCommandList::EndDebugLabel()
	{
		VK_END_DEBUG_LABEL(m_CommandBuffer->CommandBuffer);
	}

	void VulkanCommandList::InsertDebugLabel(const String& name, const IntegerColor& color)
	{
		VK_INSERT_DEBUG_LABEL(m_CommandBuffer->CommandBuffer, name, color);
	}


	VulkanCommandBuffer* VulkanCommandList::GetCommandBuffer()
	{
		return m_CommandBuffer;
	}
}
