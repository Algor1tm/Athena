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
		ATN_CORE_ASSERT(info.Shader->IsCompute(), "Compute pipeline require compute shader!");

		m_Info = info;
		m_Hash = Math::Random::UInt64(); 	// TODO: maybe try to hash more clever way
		m_VulkanPipeline = VK_NULL_HANDLE;
		m_WorkGroupSize = m_Info.Shader->GetMetaData().WorkGroupSize;

		DescriptorSetManagerCreateInfo setManagerInfo;
		setManagerInfo.Name = info.Name;
		setManagerInfo.Shader = m_Info.Shader;
		setManagerInfo.FirstSet = 1;
		setManagerInfo.LastSet = 4;
		m_DescriptorSetManager = DescriptorSetManager(setManagerInfo);

		RecreatePipeline();

		m_Info.Shader->AddOnReloadCallback(m_Hash, [this]()
		{
			m_WorkGroupSize = m_Info.Shader->GetMetaData().WorkGroupSize;
			RecreatePipeline();
		});
	}

	VulkanComputePass::~VulkanComputePass()
	{
		m_Info.Shader->RemoveOnReloadCallback(m_Hash);
		CleanUp();
	}

	void VulkanComputePass::SetInput(const String& name, const Ref<RenderResource>& resource)
	{
		m_DescriptorSetManager.Set(name, resource);
	}

	void VulkanComputePass::Bake()
	{
		m_DescriptorSetManager.Bake();
	}

	void VulkanComputePass::Begin(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		if (m_Info.DebugColor != LinearColor(0.f))
			Renderer::BeginDebugRegion(commandBuffer, m_Info.Name, m_Info.DebugColor);

		Renderer::Submit([this, commandBuffer]()
		{
			VkCommandBuffer cmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

			for (const auto& image : m_Outputs)
			{
				bool isAttachment = image->GetInfo().Usage & ImageUsage::ATTACHMENT;

				VkImageLayout newLayout = VK_IMAGE_LAYOUT_GENERAL;
				VkPipelineStageFlags sourceStage = isAttachment ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

				image.As<VulkanImage>()->RT_TransitionLayout(cmdBuffer, newLayout, sourceStage, destinationStage);
			}

			if (m_Info.Shader->IsCompiled())
			{
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_VulkanPipeline);

				m_DescriptorSetManager.RT_InvalidateAndUpdate();
				m_DescriptorSetManager.RT_BindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE);
			}
		});
	}

	void VulkanComputePass::End(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		Renderer::Submit([this, commandBuffer]()
		{
			VkCommandBuffer cmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

			for (const auto& image : m_Outputs)
			{
				bool isAttachment = image->GetInfo().Usage & ImageUsage::ATTACHMENT;

				VkImageLayout newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

				image.As<VulkanImage>()->RT_TransitionLayout(cmdBuffer, newLayout, sourceStage, destinationStage);
			}
		});

		if (m_Info.DebugColor != LinearColor(0.f))
			Renderer::EndDebugRegion(commandBuffer);
	}

	void VulkanComputePass::RT_SetPushConstants(VkCommandBuffer commandBuffer, const Ref<Material>& material)
	{
		if (m_PushConstantStageFlags != 0)
		{
			vkCmdPushConstants(commandBuffer,
				m_PipelineLayout,
				m_PushConstantStageFlags,
				0,
				m_PushConstantSize,
				material->RT_GetPushConstantData());
		}
	}

	void VulkanComputePass::RecreatePipeline()
	{
		CleanUp();

		if (!m_Info.Shader->IsCompiled())
			return;

		const auto& pushConstant = m_Info.Shader->GetMetaData().PushConstant;
		m_PushConstantStageFlags = Vulkan::GetShaderStageFlags(pushConstant.StageFlags);
		m_PushConstantSize = pushConstant.Size;
		m_PipelineLayout = m_Info.Shader.As<VulkanShader>()->GetPipelineLayout();

		Renderer::Submit([this]()
		{
			auto vkShader = m_Info.Shader.As<VulkanShader>();

			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfo.stage = vkShader->GetPipelineStages()[0];
			pipelineInfo.layout = m_PipelineLayout;

			VK_CHECK(vkCreateComputePipelines(VulkanContext::GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VulkanPipeline));
			Vulkan::SetObjectDebugName(m_VulkanPipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, m_Info.Name);
		});
	}

	void VulkanComputePass::CleanUp()
	{
		if (m_VulkanPipeline != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([pipeline = m_VulkanPipeline]()
			{
				vkDestroyPipeline(VulkanContext::GetLogicalDevice(), pipeline, nullptr);
			});
		}
	}
}
