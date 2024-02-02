#include "VulkanShader.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanShader::VulkanShader(const FilePath& path, const String& name)
	{
		m_FilePath = path;
		m_Name = name;
		CompileOrGetFromCache(Renderer::GetConfig().ForceCompileShaderPack);
	}

	VulkanShader::VulkanShader(const FilePath& path)
		: VulkanShader(path, path.stem().string())
	{

	}

	VulkanShader::~VulkanShader()
	{
		CleanUp();
	}

	void VulkanShader::Reload()
	{
		CleanUp();
		CompileOrGetFromCache(true);
	}

	void VulkanShader::CompileOrGetFromCache(bool forceCompile)
	{
		ShaderCompiler compiler(m_FilePath, m_Name);
		m_IsCompiled = compiler.CompileOrGetFromCache(forceCompile);
		m_ReflectionData = compiler.Reflect();

		if (m_IsCompiled)
			CreateVulkanShaderModulesAndStages(compiler);

		struct DescriptorSetLayoutStatistics
		{
			uint32 UBOs;
			uint32 Samplers;
			uint32 Textures2D;
		};

		std::unordered_map<uint32, DescriptorSetLayoutStatistics> stats;
		std::unordered_map<uint32, std::vector<VkDescriptorSetLayoutBinding>> bindings;

		for (const auto& [name, ubo] : m_ReflectionData.UniformBuffers)
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			uboLayoutBinding.binding = ubo.Binding;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.descriptorCount = 1;
			uboLayoutBinding.stageFlags = Vulkan::GetShaderStageFlags(ubo.StageFlags);
			uboLayoutBinding.pImmutableSamplers = nullptr;

			bindings[ubo.Set].push_back(uboLayoutBinding);
			stats[ubo.Set].UBOs++;
		}
		for (const auto& [name, texture] : m_ReflectionData.Textures2D)
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			uboLayoutBinding.binding = texture.Binding;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			uboLayoutBinding.descriptorCount = 1;
			uboLayoutBinding.stageFlags = Vulkan::GetShaderStageFlags(texture.StageFlags);
			uboLayoutBinding.pImmutableSamplers = nullptr;

			bindings[texture.Set].push_back(uboLayoutBinding);
			stats[texture.Set].Textures2D++;
			stats[texture.Set].Samplers++;
		}

		uint32 setsCount = 0;
		for (const auto& [set, _] : bindings)
		{
			if (setsCount < set + 1)
				setsCount = set + 1;
		}

		m_DescriptorSetLayouts.resize(setsCount);
		for (uint32 set = 0; set < setsCount; ++set)
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = bindings[set].size();
			layoutInfo.pBindings = bindings[set].data();

			VK_CHECK(vkCreateDescriptorSetLayout(VulkanContext::GetLogicalDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayouts[set]));
			const auto& setStats = stats[set];
			ATN_CORE_INFO_TAG("Renderer", "Create descriptor set layout {} with {} ubos, {} samplers, {} textures", set, setStats.UBOs, setStats.Samplers, setStats.Textures2D);
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = m_DescriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = m_DescriptorSetLayouts.data();

		VkPushConstantRange range = {};
		const auto& pushConstant = m_ReflectionData.PushConstant;

		if (pushConstant.Enabled)
		{
			range.offset = 0;
			range.size = pushConstant.Size;
			range.stageFlags = Vulkan::GetShaderStageFlags(pushConstant.StageFlags);

			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &range;
		}
		else
		{
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges = nullptr;
		}

		VK_CHECK(vkCreatePipelineLayout(VulkanContext::GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));
		Vulkan::SetObjectName(m_PipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, std::format("{}Layout", m_Name));
	}

	void VulkanShader::CreateVulkanShaderModulesAndStages(const ShaderCompiler& compiler)
	{
		const ShaderBinaries& binaries = compiler.GetBinaries();

		std::unordered_map<ShaderStage, std::string_view> debugNames;
		debugNames[ShaderStage::VERTEX_STAGE] = "Vertex";
		debugNames[ShaderStage::FRAGMENT_STAGE] = "Fragment";
		debugNames[ShaderStage::GEOMETRY_STAGE] = "Geometry";
		debugNames[ShaderStage::COMPUTE_STAGE] = "Compute";
		for (const auto& [stage, src] : binaries)
		{
			VkShaderModuleCreateInfo moduleCreateInfo = {};
			moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			moduleCreateInfo.codeSize = src.size() * sizeof(uint32);
			moduleCreateInfo.pCode = src.data();

			VK_CHECK(vkCreateShaderModule(VulkanContext::GetLogicalDevice(), &moduleCreateInfo, nullptr, &m_VulkanShaderModules[stage]));
			Vulkan::SetObjectName(m_VulkanShaderModules[stage], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, std::format("{}_{}", m_Name, debugNames[stage]));

			VkPipelineShaderStageCreateInfo shaderStageInfo = {};
			shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageInfo.stage = Vulkan::GetShaderStage(stage);
			shaderStageInfo.module = m_VulkanShaderModules.at(stage);
			shaderStageInfo.pName = compiler.GetEntryPoint(stage).data();

			m_PipelineShaderStages.push_back(shaderStageInfo);
		}
	}

	void VulkanShader::CleanUp()
	{
		Renderer::SubmitResourceFree([shaderModules = m_VulkanShaderModules, setLayouts = m_DescriptorSetLayouts, pipelineLayout = m_PipelineLayout]()
		{
			for (const auto& [stage, src] : shaderModules)
				vkDestroyShaderModule(VulkanContext::GetLogicalDevice(), shaderModules.at(stage), nullptr);

			for(const auto& setLayout : setLayouts)
				vkDestroyDescriptorSetLayout(VulkanContext::GetLogicalDevice(), setLayout, nullptr);

			vkDestroyPipelineLayout(VulkanContext::GetLogicalDevice(), pipelineLayout, nullptr);
		});
	}
}
