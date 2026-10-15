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

	}

	void VulkanCommandList::Submit(bool wait)
	{
		VulkanRHI::Get()->GetCmdBufferManager()->Submit(m_CommandBuffer, wait);
		m_CommandBuffer = nullptr;
	}
}
