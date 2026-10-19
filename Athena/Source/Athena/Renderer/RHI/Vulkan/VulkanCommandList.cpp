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
}
