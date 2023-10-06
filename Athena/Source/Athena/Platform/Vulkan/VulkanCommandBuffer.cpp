#include "VulkanCommandBuffer.h"

#include "Athena/Renderer/Renderer.h"

#include "Athena/Platform/Vulkan/VulkanDebug.h"
#include "Athena/Platform/Vulkan/VulkanRenderer.h"


namespace Athena
{
	VulkanCommandBuffer::VulkanCommandBuffer()
	{
		VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
		cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufAllocInfo.commandPool = VulkanContext::GetCommandPool();
		cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufAllocInfo.commandBufferCount = Renderer::FramesInFlight();

		m_CommandBuffers.resize(Renderer::FramesInFlight());
		VK_CHECK(vkAllocateCommandBuffers(VulkanContext::GetCurrentDevice()->GetLogicalDevice(), &cmdBufAllocInfo, m_CommandBuffers.data()));
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{

	}

	void VulkanCommandBuffer::Begin()
	{
		vkResetCommandBuffer(m_CommandBuffers[Renderer::CurrentFrameIndex()], 0);

		VkCommandBufferBeginInfo cmdBufBeginInfo = {};
		cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK(vkBeginCommandBuffer(m_CommandBuffers[Renderer::CurrentFrameIndex()], &cmdBufBeginInfo));
	}

	void VulkanCommandBuffer::End()
	{
		VK_CHECK(vkEndCommandBuffer(m_CommandBuffers[Renderer::CurrentFrameIndex()]));
	}

	VkCommandBuffer VulkanCommandBuffer::GetNativeCmdBuffer()
	{
		return m_CommandBuffers[Renderer::CurrentFrameIndex()];
	}
}
