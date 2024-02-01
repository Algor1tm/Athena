#include "VulkanMaterial.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"


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

	void VulkanMaterial::Set(std::string_view name, const Ref<ShaderResource>& resource)
	{
		m_DescriptorSetManager->Set(name, resource);
	}

	void VulkanMaterial::Set(std::string_view name, const Matrix4& value)
	{
		Renderer::Submit([this, name, value]()
		{
			RT_SetPushConstantData(name, ShaderDataType::Mat4, &value);
		});
	}

	void VulkanMaterial::Set(std::string_view name, const Vector4& value)
	{
		Renderer::Submit([this, name, value]()
		{
			RT_SetPushConstantData(name, ShaderDataType::Float4, &value);
		});
	}

	void VulkanMaterial::Set(std::string_view name, float value)
	{
		Renderer::Submit([this, name, value]()
		{
			RT_SetPushConstantData(name, ShaderDataType::Float, &value);
		});
	}

	void VulkanMaterial::Set(std::string_view name, uint32 value)
	{
		Renderer::Submit([this, name, value]()
		{
			RT_SetPushConstantData(name, ShaderDataType::UInt, &value);
		});
	}

	void VulkanMaterial::RT_SetPushConstantData(std::string_view name, ShaderDataType dataType, const void* data)
	{
		const auto& pushConstantData = m_Shader->GetReflectionData().PushConstant;

		String nameStr = String(name);
		if (!pushConstantData.Members.contains(nameStr))
		{
			ATN_CORE_ERROR_TAG("Renderer", "Failed to set shader push constant member with name '{}' (invalid name)", name);
			return;
		}

		const auto& memberData = pushConstantData.Members.at(nameStr);
		if (memberData.Type != dataType)
		{
			ATN_CORE_ERROR_TAG("Renderer", "Failed to set shader push constant member with name '{}' \
					(type is not matching: given - '{}', expected - '{}')", name, ShaderDataTypeToString(memberData.Type), ShaderDataTypeToString(dataType));
			return;
		}

		memcpy(&m_PushConstantBuffer[memberData.Offset], data, memberData.Size);
	}

	void VulkanMaterial::Bind()
	{
		Renderer::Submit([this]()
		{
			m_DescriptorSetManager->RT_InvalidateAndUpdate();
			m_DescriptorSetManager->RT_BindDescriptorSets();
		});
	}

	void VulkanMaterial::RT_UpdateForRendering()
	{
		if (m_Shader->GetReflectionData().PushConstant.Enabled)
		{
			const auto& pushConstant = m_Shader->GetReflectionData().PushConstant;
			vkCmdPushConstants(VulkanContext::GetActiveCommandBuffer(), 
				m_Shader.As<VulkanShader>()->GetPipelineLayout(),
				Vulkan::GetShaderStageFlags(pushConstant.StageFlags),
				0,
				pushConstant.Size,
				m_PushConstantBuffer);
		}
	}
}
