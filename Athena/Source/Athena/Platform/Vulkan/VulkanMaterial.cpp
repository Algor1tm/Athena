#include "VulkanMaterial.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Athena/Platform/Vulkan/VulkanTexture2D.h"
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

				m_ResourcesTable[ubo.Set][name] = resourceDesc;
			}
			for (const auto& [name, texture] : m_Shader->GetReflectionData().SampledTextures)
			{
				ResourceDescription resourceDesc;
				resourceDesc.Resource = nullptr;
				resourceDesc.Binding = texture.Binding;

				m_ResourcesTable[texture.Set][name] = resourceDesc;
			}

			const auto& setLayouts = m_Shader.As<VulkanShader>()->GetAllDescriptorSetLayouts();
			m_DescriptorSets.resize(Renderer::GetFramesInFlight());
			for (uint32 frameIndex = 0; frameIndex < Renderer::GetFramesInFlight(); ++frameIndex)
			{
				VkDescriptorSetAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = VulkanContext::GetDescriptorPool();
				allocInfo.descriptorSetCount = setLayouts.size();
				allocInfo.pSetLayouts = setLayouts.data();

				m_DescriptorSets[frameIndex].resize(setLayouts.size());
				VK_CHECK(vkAllocateDescriptorSets(VulkanContext::GetLogicalDevice(), &allocInfo, m_DescriptorSets[frameIndex].data()));
			}
		});
	}

	VulkanMaterial::~VulkanMaterial()
	{

	}

	void VulkanMaterial::Set(std::string_view name, const Ref<ShaderResource>& resource)
	{
		Renderer::Submit([this, resource, name]()
		{
			bool findResource = false;
			for (auto& [set, setData] : m_ResourcesTable)
			{
				if (!setData.contains(name))
					continue;

				setData.at(name).Resource = resource;
				ResourceDescription resourceDesc = setData.at(name);

				for (uint32 frameIndex = 0; frameIndex < Renderer::GetFramesInFlight(); ++frameIndex)
				{
					VkWriteDescriptorSet descriptorWrite = {};
					descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite.dstSet = m_DescriptorSets[frameIndex][set];
					descriptorWrite.dstBinding = resourceDesc.Binding;
					descriptorWrite.dstArrayElement = 0;
					descriptorWrite.descriptorCount = 1;

					switch (resourceDesc.Resource->GetResourceType())
					{
					case ShaderResourceType::UniformBuffer:
					{
						Ref<VulkanUniformBuffer> ubo = resourceDesc.Resource.As<VulkanUniformBuffer>();

						VkDescriptorBufferInfo bufferInfo = {};
						bufferInfo.buffer = ubo->GetVulkanBuffer(frameIndex);
						bufferInfo.offset = 0;
						bufferInfo.range = ubo->GetSize();

						descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						descriptorWrite.pBufferInfo = &bufferInfo;

						vkUpdateDescriptorSets(VulkanContext::GetLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
						break;
					}
					case ShaderResourceType::SampledTexture:
					{
						Ref<VulkanTexture2D> texture = resourceDesc.Resource.As<VulkanTexture2D>();

						VkDescriptorImageInfo imageInfo = {};
						imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						imageInfo.imageView = texture->GetVulkanImageView();
						imageInfo.sampler = texture->GetVulkanSampler();

						descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						descriptorWrite.pImageInfo = &imageInfo;

						vkUpdateDescriptorSets(VulkanContext::GetLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
						break;
					}
					}
				}

				findResource = true;
				break;
			}

			if (!findResource)
			{
				ATN_CORE_ERROR_TAG("Renderer", "Failed to set shader resource with name '{}' (could not find resource with that name)", name);
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
		uint32 setCount = m_DescriptorSets[0].size();

		if (setCount > 1)
		{
			vkCmdBindDescriptorSets(
				VulkanContext::GetActiveCommandBuffer(),
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_Shader.As<VulkanShader>()->GetPipelineLayout(),
				1, setCount - 1,
				&m_DescriptorSets[Renderer::GetCurrentFrameIndex()][1],
				0, 0);
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

		uint32 setCount = m_DescriptorSets[0].size();

		if (setCount > 0)
		{
			vkCmdBindDescriptorSets(
				VulkanContext::GetActiveCommandBuffer(),
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_Shader.As<VulkanShader>()->GetPipelineLayout(),
				0, 1,
				&m_DescriptorSets[Renderer::GetCurrentFrameIndex()][0],
				0, 0);
		}
	}
}
