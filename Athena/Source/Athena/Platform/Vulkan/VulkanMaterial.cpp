#include "VulkanMaterial.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"
#include "Athena/Platform/Vulkan/VulkanRenderCommandBuffer.h"


namespace Athena
{
	VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader, const String& name)
	{
		m_Shader = shader;
		m_Name = name;
		memset(m_PushConstantBuffer, 0, sizeof(m_PushConstantBuffer));

		DescriptorSetManagerCreateInfo info;
		info.Name = name;
		info.Shader = m_Shader;
		info.FirstSet = 0;
		info.LastSet = 0;
		m_DescriptorSetManager = Ref<DescriptorSetManager>::Create(info);
		m_DescriptorSetManager->Bake();
	}

	VulkanMaterial::~VulkanMaterial()
	{
		
	}

	void VulkanMaterial::Set(const String& name, const Ref<ShaderResource>& resource)
	{
		m_DescriptorSetManager->Set(name, resource);
	}

	Ref<Texture2D> VulkanMaterial::GetTexture(const String& name)
	{
		return m_DescriptorSetManager->Get<Ref<Texture2D>>(name);
	}

	void VulkanMaterial::SetInternal(const String& name, ShaderDataType dataType, const void* data)
	{
		if (Exists(name, dataType))
		{
			const auto& pushConstantData = m_Shader->GetReflectionData().PushConstant;
			const auto& memberData = pushConstantData.Members.at(name);
			memcpy(&m_PushConstantBuffer[memberData.Offset], data, memberData.Size);
		}
	}

	bool VulkanMaterial::GetInternal(const String& name, ShaderDataType dataType, void** data)
	{
		if (Exists(name, dataType))
		{
			const auto& pushConstantData = m_Shader->GetReflectionData().PushConstant;
			const auto& memberData = pushConstantData.Members.at(name);
			*data = &m_PushConstantBuffer[memberData.Offset];
			return true;
		}

		return false;
	}

	bool VulkanMaterial::Exists(const String& name, ShaderDataType dataType)
	{
		const auto& pushConstantData = m_Shader->GetReflectionData().PushConstant;

		if (!pushConstantData.Members.contains(name))
		{
			ATN_CORE_ERROR_TAG("Renderer", "Failed to get or set shader push constant member with name '{}' (invalid name)", name);
			return false;
		}

		const auto& memberData = pushConstantData.Members.at(name);
		if (memberData.Type != dataType)
		{
			ATN_CORE_ERROR_TAG("Renderer", "Failed to get or set shader push constant member with name '{}' \
					(type is not matching: given - '{}', expected - '{}')", name, ShaderDataTypeToString(memberData.Type), ShaderDataTypeToString(dataType));
			return false;
		}

		return true;
	}

	void VulkanMaterial::Bind(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		Renderer::Submit([this, commandBuffer]()
		{
			VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();

			m_DescriptorSetManager->RT_InvalidateAndUpdate();
			m_DescriptorSetManager->RT_BindDescriptorSets(vkcmdBuffer);
		});
	}

	void VulkanMaterial::RT_UpdateForRendering(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		if (m_Shader->GetReflectionData().PushConstant.Enabled)
		{
			VkCommandBuffer vkcmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer();
			const auto& pushConstant = m_Shader->GetReflectionData().PushConstant;

			vkCmdPushConstants(vkcmdBuffer,
				m_Shader.As<VulkanShader>()->GetPipelineLayout(),
				Vulkan::GetShaderStageFlags(pushConstant.StageFlags),
				0,
				pushConstant.Size,
				m_PushConstantBuffer);
		}
	}
}
