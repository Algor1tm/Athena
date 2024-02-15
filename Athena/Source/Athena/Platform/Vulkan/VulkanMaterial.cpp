#include "VulkanMaterial.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"


namespace Athena
{
	VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader, const String& name)
		: Material(shader, name)
	{
		DescriptorSetManagerCreateInfo info;
		info.Name = name;
		info.Shader = shader;
		info.FirstSet = 0;
		info.LastSet = 0;
		m_DescriptorSetManager = Ref<DescriptorSetManager>::Create(info);
		m_DescriptorSetManager->Bake();

		m_PipelineBindPoint = shader->IsCompute() ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
	}

	VulkanMaterial::~VulkanMaterial()
	{
		
	}

	void VulkanMaterial::SetResource(const String& name, const Ref<RenderResource>& resource, uint32 arrayIndex)
	{
		m_DescriptorSetManager->Set(name, resource, arrayIndex);
	}

	Ref<RenderResource> VulkanMaterial::GetResource(const String& name)
	{
		return m_DescriptorSetManager->Get(name);
	}

	void VulkanMaterial::Bind(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		Renderer::Submit([this, commandBuffer]()
		{
			VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

			m_DescriptorSetManager->RT_InvalidateAndUpdate();
			m_DescriptorSetManager->RT_BindDescriptorSets(vkcmdBuffer, m_PipelineBindPoint);
		});
	}

	void VulkanMaterial::RT_SetPushConstant(const Ref<RenderCommandBuffer>& commandBuffer, const PushConstantRange& range)
	{
		Ref<Shader> shader = GetShader();

		if (shader->GetMetaData().PushConstant.Enabled)
		{
			VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();
			const auto& pushConstant = shader->GetMetaData().PushConstant;

			vkCmdPushConstants(vkcmdBuffer,
				shader.As<VulkanShader>()->GetPipelineLayout(),
				Vulkan::GetShaderStageFlags(pushConstant.StageFlags),
				0,
				pushConstant.Size,
				range.GetData());
		}
	}
}
