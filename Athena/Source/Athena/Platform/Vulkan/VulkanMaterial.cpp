#include "VulkanMaterial.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Athena/Platform/Vulkan/VulkanShader.h"


namespace Athena
{
	VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader)
	{
		m_Shader = shader;
		memset(m_PushConstantBuffer, 0, sizeof(m_PushConstantBuffer));

		Renderer::Submit([this]()
		{
			for (const auto& [name, ubo] : m_Shader->GetReflectionData().UniformBuffers)
			{
				ResourceDescription resourceDesc;
				resourceDesc.Resource = nullptr;
				resourceDesc.Binding = ubo.Binding;

				m_ResourcesTable[name] = resourceDesc;
			}

			Ref<VulkanShader> vkShader = m_Shader.As<VulkanShader>();
			std::vector<VkDescriptorSetLayout> layouts(Renderer::GetFramesInFlight(), vkShader->GetDescriptorSetLayout());

			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = VulkanContext::GetDescriptorPool();
			allocInfo.descriptorSetCount = Renderer::GetFramesInFlight();
			allocInfo.pSetLayouts = layouts.data();

			m_DescriptorSets.resize(Renderer::GetFramesInFlight());
			VK_CHECK(vkAllocateDescriptorSets(VulkanContext::GetLogicalDevice(), &allocInfo, m_DescriptorSets.data()));
		});
	}

	VulkanMaterial::~VulkanMaterial()
	{

	}

	void VulkanMaterial::Set(std::string_view name, const Ref<ShaderResource>& resource)
	{
		Renderer::Submit([this, resource, name]()
		{
			if (!m_ResourcesTable.contains(name))
			{
				ATN_CORE_ERROR_TAG("Renderer", "Failed to set shader resource with name '{}' (could not find resource with that name)", name);
				return;
			}

			m_ResourcesTable.at(name).Resource = resource;

			for (uint32 i = 0; i < Renderer::GetFramesInFlight(); ++i)
			{
				RT_UpdateDescriptorSet(m_ResourcesTable.at(name), i);
			}
		});
	}

	void VulkanMaterial::Set(std::string_view name, const Matrix4& mat4)
	{
		Renderer::Submit([this, name, mat4]()
		{
			const auto& pushConstantData = m_Shader->GetReflectionData().PushConstant;

			String nameStr = String(name);
			if (!pushConstantData.Members.contains(nameStr))
			{
				ATN_CORE_ERROR_TAG("Renderer", "Failed to set shader push constant member with name '{}' (could not find member with that name)", name);
				return;
			}

			const auto& memberData = pushConstantData.Members.at(nameStr);
			if (memberData.Type != ShaderDataType::Mat4)
			{
				ATN_CORE_ERROR_TAG("Renderer", "Failed to set shader push constant member with name '{}' \
					(type is not matching: given - '{}', expected - '{}')", name, ShaderDataTypeToString(memberData.Type), ShaderDataTypeToString(ShaderDataType::Mat4));
				return;
			}

			memcpy(&m_PushConstantBuffer[memberData.Offset], mat4.Data(), memberData.Size);
		});
	}

	void VulkanMaterial::RT_Bind()
	{
		vkCmdBindDescriptorSets(
			VulkanContext::GetActiveCommandBuffer(), 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			m_Shader.As<VulkanShader>()->GetPipelineLayout(),
			0, 1,
			&m_DescriptorSets[Renderer::GetCurrentFrameIndex()], 
			0, 0);
	}

	void VulkanMaterial::RT_UpdateDescriptorSet(const ResourceDescription& resourceDesc, uint32 frameIndex)
	{
		switch (resourceDesc.Resource->GetResourceType())
		{
		case ShaderResourceType::UniformBuffer:
		{
			Ref<VulkanUniformBuffer> ubo = resourceDesc.Resource.As<VulkanUniformBuffer>();

			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = ubo->GetVulkanBuffer(frameIndex);
			bufferInfo.offset = 0;
			bufferInfo.range = ubo->GetSize();

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_DescriptorSets[frameIndex];
			descriptorWrite.dstBinding = resourceDesc.Binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(VulkanContext::GetLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
		}
		}
	}

	void VulkanMaterial::RT_UpdateForRendering()
	{
		if (m_Shader->GetReflectionData().PushConstant.Enabled)
		{
			const auto& pushConstant = m_Shader->GetReflectionData().PushConstant;
			vkCmdPushConstants(VulkanContext::GetActiveCommandBuffer(), 
				m_Shader.As<VulkanShader>()->GetPipelineLayout(),
				VulkanUtils::GetShaderStageFlags(pushConstant.StageFlags),
				0,
				pushConstant.Size,
				m_PushConstantBuffer);
		}
	}
}
