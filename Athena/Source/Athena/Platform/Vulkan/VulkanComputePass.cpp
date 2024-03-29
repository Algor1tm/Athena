#include "VulkanComputePass.h"
#include "Athena/Math/Random.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanImage.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanTextureCube.h"
#include "Athena/Platform/Vulkan/VulkanStorageBuffer.h"
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

		VkCommandBuffer cmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();

		for (uint32 i = 0; i < m_Outputs.size(); ++i)
		{
			BarrierInfo barrier = m_Barriers[i];
			Ref<RenderResource> output = m_Outputs[i];

			switch (output->GetResourceType())
			{
			case RenderResourceType::Texture2D:
			{
				Ref<VulkanImage> image = output.As<Texture2D>()->GetImage().As<VulkanImage>();
				image->TransitionLayout(cmdBuffer,
					VK_IMAGE_LAYOUT_GENERAL,
					barrier.SrcAccess, VK_ACCESS_SHADER_WRITE_BIT,
					barrier.SrcStageFlags, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

				break;
			}
			case RenderResourceType::TextureCube:
			{
				Ref<VulkanImage> image = output.As<Texture2D>()->GetImage().As<VulkanImage>();
				image->TransitionLayout(cmdBuffer,
					VK_IMAGE_LAYOUT_GENERAL,
					barrier.SrcAccess, VK_ACCESS_SHADER_WRITE_BIT,
					barrier.SrcStageFlags, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

				break;
			}
			// TODO: for now hard coded for light culling pass (synchonize access to depth attachment)
			case RenderResourceType::StorageBuffer:
			{
				VkMemoryBarrier memoryBarrier = {};
				memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				memoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; 

				vkCmdPipelineBarrier(cmdBuffer,
					VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
					VK_DEPENDENCY_BY_REGION_BIT,
					1, &memoryBarrier,
					0, nullptr,
					0, nullptr );

				break;
			}
			}
		}
	}

	void VulkanComputePass::End(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		VkCommandBuffer cmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();

		for (uint32 i = 0; i < m_Outputs.size(); ++i)
		{
			Ref<RenderResource> output = m_Outputs[i];

			switch (output->GetResourceType())
			{
			case RenderResourceType::Texture2D:
			{
				Ref<VulkanImage> image = output.As<Texture2D>()->GetImage().As<VulkanImage>();
				image->TransitionLayout(cmdBuffer,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

				break;
			}
			case RenderResourceType::TextureCube:
			{
				Ref<VulkanImage> image = output.As<Texture2D>()->GetImage().As<VulkanImage>();
				image->TransitionLayout(cmdBuffer,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

				break;
			}
			case RenderResourceType::StorageBuffer:
			{
				VkMemoryBarrier memBarrier = {};
				memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(
					cmdBuffer,
					VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_DEPENDENCY_BY_REGION_BIT,
					1, &memBarrier,
					0, nullptr,
					0, nullptr
				);
			}
			}
		}

		if (m_Info.DebugColor != LinearColor(0.f))
			Renderer::EndDebugRegion(commandBuffer);
	}

	void VulkanComputePass::Bake()
	{
		ATN_CORE_ASSERT(!(m_Info.InputRenderPass && m_Info.InputComputePass));

		for (const auto& output : m_Outputs)
		{
			switch (output->GetResourceType())
			{
			case RenderResourceType::TextureCube:
			{
				m_Barriers.push_back({ VK_ACCESS_NONE, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });
				break;
			}
			case RenderResourceType::StorageBuffer:
			{
				m_Barriers.push_back({ VK_ACCESS_NONE, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });
				break;
			}
			case RenderResourceType::Texture2D:
			{
				Ref<Texture2D> outputTarget = output;
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
				else
				{
					m_Barriers.push_back({ VK_ACCESS_NONE, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });
				}

				break;
			}
			}
		}
	}
}
