#include "VulkanComputePipeline.h"
#include "Athena/Math/Random.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"


namespace Athena
{
	VulkanComputePipeline::VulkanComputePipeline(const ComputePipelineCreateInfo& info)
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

	VulkanComputePipeline::~VulkanComputePipeline()
	{
		m_Info.Shader->RemoveOnReloadCallback(m_Hash);
		CleanUp();
	}

	bool VulkanComputePipeline::Bind(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		if (!m_Info.Shader->IsCompiled())
			return false;

		VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();
		vkCmdBindPipeline(vkcmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_VulkanPipeline);

		m_DescriptorSetManager.InvalidateAndUpdate();
		m_DescriptorSetManager.BindDescriptorSets(vkcmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE);

		return true;
	}

	void VulkanComputePipeline::SetInput(const String& name, const Ref<RenderResource>& resource)
	{
		m_DescriptorSetManager.Set(name, resource);
	}

	void VulkanComputePipeline::Bake()
	{
		m_DescriptorSetManager.Bake();
	}

	Vector3u VulkanComputePipeline::GetWorkGroupSize() const
	{
		return m_WorkGroupSize;
	}

	void VulkanComputePipeline::CleanUp()
	{
		if (m_VulkanPipeline != VK_NULL_HANDLE)
		{
			Renderer::SubmitResourceFree([pipeline = m_VulkanPipeline]()
			{
				vkDestroyPipeline(VulkanContext::GetLogicalDevice(), pipeline, nullptr);
			});
		}
	}

	void VulkanComputePipeline::RT_SetPushConstants(VkCommandBuffer commandBuffer, const Ref<Material>& material)
	{
		if (m_PushConstantStageFlags != 0)
		{
			vkCmdPushConstants(commandBuffer,
				m_PipelineLayout,
				m_PushConstantStageFlags,
				0,
				m_PushConstantSize,
				material->GetPushConstantData());
		}
	}

	void VulkanComputePipeline::RecreatePipeline()
	{
		CleanUp();

		if (!m_Info.Shader->IsCompiled())
			return;

		const auto& pushConstant = m_Info.Shader->GetMetaData().PushConstant;
		m_PushConstantStageFlags = Vulkan::GetShaderStageFlags(pushConstant.StageFlags);
		m_PushConstantSize = pushConstant.Size;
		m_PipelineLayout = m_Info.Shader.As<VulkanShader>()->GetPipelineLayout();

		auto vkShader = m_Info.Shader.As<VulkanShader>();

		VkComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = vkShader->GetPipelineStages()[0];
		pipelineInfo.layout = m_PipelineLayout;

		VK_CHECK(vkCreateComputePipelines(VulkanContext::GetLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_VulkanPipeline));
		Vulkan::SetObjectDebugName(m_VulkanPipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, m_Info.Name);
	}
}
