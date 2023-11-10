#include "VulkanMaterial.h"

#include "Athena/Platform/Vulkan/VulkanUtils.h"


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
			}

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = bindings.size();
			layoutInfo.pBindings = bindings.data();

			VK_CHECK(vkCreateDescriptorSetLayout(VulkanContext::GetLogicalDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout));
		});
	}

	VulkanMaterial::~VulkanMaterial()
	{
		Renderer::SubmitResourceFree([setLayout = m_DescriptorSetLayout]()
		{
			vkDestroyDescriptorSetLayout(VulkanContext::GetLogicalDevice(), setLayout, nullptr);
		});
	}

	void VulkanMaterial::RT_UpdateForRendering()
	{

	}
}
