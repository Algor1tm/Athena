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
		cmdBufAllocInfo.commandBufferCount = Renderer::GetFramesInFlight();

		m_CommandBuffers.resize(Renderer::GetFramesInFlight());
		VK_CHECK(vkAllocateCommandBuffers(VulkanContext::GetDevice()->GetLogicalDevice(), &cmdBufAllocInfo, m_CommandBuffers.data()));
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{

	}

	void VulkanCommandBuffer::Begin()
	{
		VK_CHECK(vkResetCommandBuffer(m_CommandBuffers[Renderer::GetCurrentFrameIndex()], 0));

		VkCommandBufferBeginInfo cmdBufBeginInfo = {};
		cmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK(vkBeginCommandBuffer(m_CommandBuffers[Renderer::GetCurrentFrameIndex()], &cmdBufBeginInfo));
	}

	void VulkanCommandBuffer::End()
	{
		VK_CHECK(vkEndCommandBuffer(m_CommandBuffers[Renderer::GetCurrentFrameIndex()]));
	}

	VkCommandBuffer VulkanCommandBuffer::GetNativeCmdBuffer()
	{
		return m_CommandBuffers[Renderer::GetCurrentFrameIndex()];
	}
}
