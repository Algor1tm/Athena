#include "VulkanComputePass.h"
#include "Athena/Math/Random.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"


namespace Athena
{
	VulkanComputePass::VulkanComputePass(const ComputePassCreateInfo& info)
	{
		m_Info = info;
	}

	VulkanComputePass::~VulkanComputePass()
	{

	}

	void VulkanComputePass::Begin(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		if (m_Info.DebugColor != LinearColor(0.f))
			Renderer::BeginDebugRegion(commandBuffer, m_Info.Name, m_Info.DebugColor);

		VkCommandBuffer cmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

		for (uint32 i = 0; i < m_Texture2DOutputs.size(); ++i)
		{
			VkAccessFlags srcAccess = VK_ACCESS_NONE;
			VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			if (!m_Barriers.empty())
			{
				srcAccess = m_Barriers[i].SrcAccess;
				srcStage = m_Barriers[i].SrcStageFlags;
			}

			Ref<VulkanImage> image = m_Texture2DOutputs[i]->GetImage().As<VulkanImage>();
			image->TransitionLayout(cmdBuffer, 
				VK_IMAGE_LAYOUT_GENERAL, 
				srcAccess, VK_ACCESS_SHADER_WRITE_BIT, 
				srcStage, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}

		for (const auto& image : m_TextureCubeOutputs)
		{
			image->GetImage().As<VulkanImage>()->TransitionLayout(cmdBuffer, 
				VK_IMAGE_LAYOUT_GENERAL, 
				VK_ACCESS_NONE, VK_ACCESS_SHADER_WRITE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}
	}

	void VulkanComputePass::End(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		VkCommandBuffer cmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

		for (const auto& image : m_Texture2DOutputs)
		{
			image->GetImage().As<VulkanImage>()->TransitionLayout(cmdBuffer,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		for (const auto& image : m_TextureCubeOutputs)
		{
			image->GetImage().As<VulkanImage>()->TransitionLayout(cmdBuffer, 
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		if (m_Info.DebugColor != LinearColor(0.f))
			Renderer::EndDebugRegion(commandBuffer);
	}

	void VulkanComputePass::Bake()
	{
		if (!m_Info.InputRenderPass && !m_Info.InputComputePass)
			return;

		ATN_CORE_ASSERT(!(m_Info.InputRenderPass && m_Info.InputComputePass));

		for (const auto& outputTarget : m_Texture2DOutputs)
		{
			if (m_Info.InputRenderPass)
			{
				bool sharedTarget = false;
				for (const auto& inputTarget : m_Info.InputRenderPass->GetAllOutputs())
				{
					if (outputTarget == inputTarget.Texture)
						sharedTarget = true;
				}

				bool colorFormat = Image::IsColorFormat(outputTarget->GetFormat());
				BarrierInfo barrier;
				if (sharedTarget)
				{
					barrier.SrcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					barrier.SrcStageFlags = colorFormat ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				}
				else
				{
					barrier.SrcAccess = VK_ACCESS_SHADER_READ_BIT;
					barrier.SrcStageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}

				m_Barriers.push_back(barrier);
			}
			else if (m_Info.InputComputePass)
			{
				bool sharedTarget = false;
				for (const auto& inputTarget : m_Info.InputComputePass->GetAllOutputs())
				{
					if (outputTarget == inputTarget)
						sharedTarget = true;
				}

				BarrierInfo barrier;
				if (sharedTarget)
				{
					barrier.SrcAccess = VK_ACCESS_SHADER_WRITE_BIT;
					barrier.SrcStageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				}
				else
				{
					barrier.SrcAccess = VK_ACCESS_SHADER_READ_BIT;
					barrier.SrcStageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				}

				m_Barriers.push_back(barrier);
			}
		}
	}
}
