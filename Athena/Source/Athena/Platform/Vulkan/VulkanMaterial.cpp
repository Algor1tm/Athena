#include "VulkanMaterial.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"


namespace Athena
{
	namespace VulkanUtils 
	{
		static VkShaderStageFlags GetShaderStageFlags(ShaderStage stageFlags)
		{
			uint64 flags = 0;

			if (stageFlags & ShaderStage::VERTEX_STAGE)
				flags |= VK_SHADER_STAGE_VERTEX_BIT;

			if (stageFlags & ShaderStage::FRAGMENT_STAGE)
				flags |= VK_SHADER_STAGE_FRAGMENT_BIT;

			if (stageFlags & ShaderStage::GEOMETRY_STAGE)
				flags |= VK_SHADER_STAGE_GEOMETRY_BIT;

			if (stageFlags & ShaderStage::COMPUTE_STAGE)
				flags |= VK_SHADER_STAGE_COMPUTE_BIT;

			return flags;
		}
	}

	VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader)
	{
		m_Shader = shader;

		Renderer::Submit([this]()
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings;

			for (const auto& [name, ubo] : m_Shader->GetReflectionData().UniformBuffers)
			{
				VkDescriptorSetLayoutBinding uboLayoutBinding = {};
				uboLayoutBinding.binding = ubo.Binding;
				uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				uboLayoutBinding.descriptorCount = 1;
				uboLayoutBinding.stageFlags = VulkanUtils::GetShaderStageFlags(ubo.StageFlags);
				uboLayoutBinding.pImmutableSamplers = nullptr;

				bindings.push_back(uboLayoutBinding);

				ResourceDescription resourceDesc;
				resourceDesc.Resource = nullptr;
				resourceDesc.Binding = ubo.Binding;

				m_ResourcesTable[name] = resourceDesc;
			}

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = bindings.size();
			layoutInfo.pBindings = bindings.data();

			VK_CHECK(vkCreateDescriptorSetLayout(VulkanContext::GetLogicalDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout));

			std::vector<VkDescriptorSetLayout> layouts(Renderer::GetFramesInFlight(), m_DescriptorSetLayout);

			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = VulkanContext::GetDescriptorPool();
			allocInfo.descriptorSetCount = Renderer::GetFramesInFlight();
			allocInfo.pSetLayouts = layouts.data();

			m_DescriptorSets.resize(Renderer::GetFramesInFlight());
			VK_CHECK(vkAllocateDescriptorSets(VulkanContext::GetLogicalDevice(), &allocInfo, m_DescriptorSets.data()));


			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges = nullptr;

			VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));
		});
	}

	VulkanMaterial::~VulkanMaterial()
	{
		Renderer::SubmitResourceFree([setLayout = m_DescriptorSetLayout, pipelineLayout = m_PipelineLayout]()
		{
			vkDestroyDescriptorSetLayout(VulkanContext::GetLogicalDevice(), setLayout, nullptr);
			vkDestroyPipelineLayout(VulkanContext::GetLogicalDevice(), pipelineLayout, nullptr);
		});
	}

	void VulkanMaterial::Set(std::string_view name, const Ref<ShaderResource>& resource)
	{
		Renderer::Submit([this, resource, name]()
		{
			if (!m_ResourcesTable.contains(name))
			{
				ATN_CORE_ERROR_TAG("Renderer", "Failed to set shader resource with name '{}'", name);
				return;
			}

			m_ResourcesTable.at(name).Resource = resource;

			for (uint32 i = 0; i < Renderer::GetFramesInFlight(); ++i)
			{
				UpdateDescriptorSet(m_ResourcesTable.at(name), i);
			}
		});
	}

	void VulkanMaterial::RT_Bind()
	{
		vkCmdBindDescriptorSets(
			VulkanContext::GetActiveCommandBuffer(), 
			VK_PIPELINE_BIND_POINT_GRAPHICS, 
			m_PipelineLayout, 
			0, 1,
			&m_DescriptorSets[Renderer::GetCurrentFrameIndex()], 
			0, 0);
	}

	void VulkanMaterial::UpdateDescriptorSet(const ResourceDescription& resourceDesc, uint32 frameIndex)
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
}
