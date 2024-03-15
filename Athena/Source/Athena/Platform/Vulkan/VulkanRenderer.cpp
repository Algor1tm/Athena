#include "VulkanRenderer.h"

#include "Athena/Core/Application.h"

#include "Athena/Platform/Vulkan/VulkanDevice.h"
#include "Athena/Platform/Vulkan/VulkanSwapChain.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"
#include "Athena/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Athena/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Athena/Platform/Vulkan/VulkanComputePipeline.h"
#include "Athena/Platform/Vulkan/VulkanPipeline.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"


namespace Athena
{
	void VulkanRenderer::Init()
	{
		VulkanContext::Init();

		// Create Command buffers
		{
			VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
			cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cmdBufAllocInfo.commandPool = VulkanContext::GetCommandPool();
			cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cmdBufAllocInfo.commandBufferCount = Renderer::GetFramesInFlight();

			m_VkCommandBuffers.resize(Renderer::GetFramesInFlight());
			VK_CHECK(vkAllocateCommandBuffers(VulkanContext::GetLogicalDevice(), &cmdBufAllocInfo, m_VkCommandBuffers.data()));
		}

		// Acquire function pointers
#ifdef VULKAN_ENABLE_DEBUG_INFO
		m_DebugMarkerBeginPFN = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(VulkanContext::GetLogicalDevice(), "vkCmdDebugMarkerBeginEXT");
		m_DebugMarkerEndPFN = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(VulkanContext::GetLogicalDevice(), "vkCmdDebugMarkerEndEXT");
		m_DebugMarkerInsertPFN = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(VulkanContext::GetLogicalDevice(), "vkCmdDebugMarkerInsertEXT");
#endif
	}
	 
	void VulkanRenderer::Shutdown()
	{
		Renderer::SubmitResourceFree([vkCommandBuffers = m_VkCommandBuffers]()
		{
			vkFreeCommandBuffers(VulkanContext::GetLogicalDevice(), VulkanContext::GetCommandPool(), vkCommandBuffers.size(), vkCommandBuffers.data());
			VulkanContext::Shutdown();
		});
	}

	void VulkanRenderer::OnUpdate()
	{
		VulkanContext::GetAllocator()->OnUpdate();
	}

	void VulkanRenderer::RenderGeometry(const Ref<RenderCommandBuffer>& commandBuffer, const Ref<Pipeline>& pipeline, const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, uint32 vertexCount)
	{
		if (!pipeline->GetInfo().Shader->IsCompiled())
			return;

		VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

		if (material)
			pipeline.As<VulkanPipeline>()->RT_SetPushConstants(vkcmdBuffer, material);
			
		Ref<VulkanVertexBuffer> vkVertexBuffer = vertexBuffer.As<VulkanVertexBuffer>();
		VkBuffer vulkanVertexBuffer = vkVertexBuffer->GetVulkanVertexBuffer();

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(vkcmdBuffer, 0, 1, &vulkanVertexBuffer, offsets);

		if (vkVertexBuffer->GetIndexBuffer())
		{
			Ref<VulkanIndexBuffer> indexBuffer = vkVertexBuffer->GetIndexBuffer().As<VulkanIndexBuffer>();
			uint32 count = vertexCount == 0 ? indexBuffer->GetCount() : vertexCount;

			vkCmdBindIndexBuffer(vkcmdBuffer, indexBuffer->GetVulkanIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(vkcmdBuffer, count, 1, 0, 0, 0);
		}
		else
		{
			uint32 stride = pipeline->GetInfo().VertexLayout.GetStride();
			uint32 vbSize = vertexBuffer->GetSize();
			uint32 count = vertexCount == 0 ? vbSize / stride : vertexCount;

			vkCmdDraw(vkcmdBuffer, count, 1, 0, 0);
		}
	}

	void VulkanRenderer::Dispatch(const Ref<RenderCommandBuffer>& commandBuffer, const Ref<ComputePipeline>& pipeline, Vector3i imageSize, const Ref<Material>& material)
	{
		if (!pipeline->GetInfo().Shader->IsCompiled())
			return;

		VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();
		Ref<VulkanComputePipeline> vkPipeline = pipeline.As<VulkanComputePipeline>();

		if (material)
			vkPipeline->RT_SetPushConstants(vkcmdBuffer, material);

		Vector3i workGroupSize = vkPipeline->GetWorkGroupSize();
		uint32 groupCountX = Math::Ceil((float)imageSize.x / workGroupSize.x);
		uint32 groupCountY = Math::Ceil((float)imageSize.y / workGroupSize.y);
		uint32 groupCountZ = Math::Ceil((float)imageSize.z / workGroupSize.z);

		vkCmdDispatch(vkcmdBuffer, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanRenderer::MemoryDependency(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		VkCommandBuffer cmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

		VkMemoryBarrier memBarrier = {};
		memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
			1, &memBarrier,
			0, nullptr,
			0, nullptr
		);
	}

	void VulkanRenderer::ExecutionDependency(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		VkCommandBuffer cmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

		vkCmdPipelineBarrier(
			cmdBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_DEPENDENCY_BY_REGION_BIT,
			0, nullptr,
			0, nullptr,
			0, nullptr
		);
	}

	void VulkanRenderer::BeginDebugRegion(const Ref<RenderCommandBuffer>& commandBuffer, std::string_view name, const Vector4& color)
	{
#ifdef VULKAN_ENABLE_DEBUG_INFO
		VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

		VkDebugMarkerMarkerInfoEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		markerInfo.color[0] = color[0];
		markerInfo.color[1] = color[1];
		markerInfo.color[2] = color[2];
		markerInfo.color[3] = color[3];
		markerInfo.pMarkerName = name.data();

		m_DebugMarkerBeginPFN(vkcmdBuffer, &markerInfo);
#endif
	}

	void VulkanRenderer::EndDebugRegion(const Ref<RenderCommandBuffer>& commandBuffer)
	{
#ifdef VULKAN_ENABLE_DEBUG_INFO
		VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();
		m_DebugMarkerEndPFN(vkcmdBuffer);
#endif
	}

	void VulkanRenderer::InsertDebugMarker(const Ref<RenderCommandBuffer>& commandBuffer, std::string_view name, const Vector4& color)
	{
#ifdef VULKAN_ENABLE_DEBUG_INFO
		VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

		VkDebugMarkerMarkerInfoEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		markerInfo.color[0] = color[0];
		markerInfo.color[1] = color[1];
		markerInfo.color[2] = color[2];
		markerInfo.color[3] = color[3];
		markerInfo.pMarkerName = name.data();

		m_DebugMarkerInsertPFN(vkcmdBuffer, &markerInfo);
#endif
	}

	void VulkanRenderer::WaitDeviceIdle()
	{
		vkDeviceWaitIdle(VulkanContext::GetDevice()->GetLogicalDevice());
	}

	void VulkanRenderer::BlitToScreen(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Image>& image)
	{
		VkCommandBuffer commandBuffer = cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

		VkImage sourceImage = image.As<VulkanImage>()->GetVulkanImage();
		VkImage swapChainImage = Application::Get().GetWindow().GetSwapChain().As<VulkanSwapChain>()->GetCurrentVulkanImage();

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		{
			barrier.image = sourceImage;
			barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			barrier.image = swapChainImage;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_NONE;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

		VkImageBlit imageBlitRegion = {};
		imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.srcSubresource.layerCount = 1;
		imageBlitRegion.srcOffsets[1] = { (int)image->GetInfo().Width, (int)image->GetInfo().Height, 1 };
		imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlitRegion.dstSubresource.layerCount = 1;
		imageBlitRegion.dstOffsets[1] = { (int)image->GetInfo().Width, (int)image->GetInfo().Height, 1 };

		vkCmdBlitImage(
			commandBuffer,
			sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			swapChainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlitRegion,
			VK_FILTER_NEAREST);


		{
			barrier.image = sourceImage;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_NONE;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);


			barrier.image = swapChainImage;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_NONE;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage, destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}
	}

	void VulkanRenderer::GetRenderCapabilities(RenderCapabilities& caps)
	{
		VulkanContext::GetDevice()->GetDeviceCapabilities(caps);
	}

	uint64 VulkanRenderer::GetMemoryUsage()
	{
		return VulkanContext::GetAllocator()->GetMemoryUsage();
	}
}
