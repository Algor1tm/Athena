#include "VulkanShader.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Platform/Vulkan/VulkanUtils.h"


namespace Athena
{
	VulkanShader::VulkanShader(const FilePath& path, const String& name)
	{
		m_FilePath = path;
		m_Name = name;
		m_PipelineLayout = VK_NULL_HANDLE;
		CompileOrGetFromCache(false);
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

		for (const auto& [_, callback] : m_OnReloadCallbacks)
		{
			callback();
		}
	}

	bool VulkanShader::IsCompute()
	{
		return m_VulkanShaderModules.contains(ShaderStage::COMPUTE_STAGE);
	}

	void VulkanShader::CompileOrGetFromCache(bool forceCompile)
	{
		ShaderCompiler compiler(m_FilePath, m_Name);
		m_IsCompiled = compiler.CompileOrGetFromCache(forceCompile);

		if (!m_IsCompiled)
			return;

		m_MetaData = compiler.Reflect();
		CreateVulkanShaderModulesAndStages(compiler);

		struct DescriptorSetLayoutStatistics
		{
			uint32 SampledTextures;
			uint32 StorageTextures;
			uint32 Samplers;
			uint32 UBOs;
			uint32 SBOs;
		};

		std::unordered_map<uint32, DescriptorSetLayoutStatistics> stats;
		std::unordered_map<uint32, std::vector<VkDescriptorSetLayoutBinding>> bindings;

		for (const auto& [name, texture] : m_MetaData.SampledTextures)
		{
			VkDescriptorSetLayoutBinding layoutBinding = {};
			layoutBinding.binding = texture.Binding;
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBinding.descriptorCount = texture.ArraySize;
			layoutBinding.stageFlags = Vulkan::GetShaderStageFlags(texture.StageFlags);
			layoutBinding.pImmutableSamplers = nullptr;
			bindings[texture.Set].push_back(layoutBinding);

			ShaderResourceDescription resourceDesc;
			resourceDesc.Type = texture.ImageType == ImageType::IMAGE_2D ? ShaderResourceType::Texture2D : ShaderResourceType::TextureCube;
			resourceDesc.Binding = texture.Binding;
			resourceDesc.Set = texture.Set;
			resourceDesc.ArraySize = texture.ArraySize;
			m_ResourcesDescriptionTable[name] = resourceDesc;

			stats[texture.Set].SampledTextures++;
		}
		for (const auto& [name, texture] : m_MetaData.StorageTextures)
		{
			VkDescriptorSetLayoutBinding layoutBinding = {};
			layoutBinding.binding = texture.Binding;
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			layoutBinding.descriptorCount = texture.ArraySize;
			layoutBinding.stageFlags = Vulkan::GetShaderStageFlags(texture.StageFlags);
			layoutBinding.pImmutableSamplers = nullptr;
			bindings[texture.Set].push_back(layoutBinding);

			ShaderResourceDescription resourceDesc;
			resourceDesc.Type = texture.ImageType == ImageType::IMAGE_2D ? ShaderResourceType::Texture2DStorage : ShaderResourceType::TextureCubeStorage;
			resourceDesc.Binding = texture.Binding;
			resourceDesc.Set = texture.Set;
			resourceDesc.ArraySize = texture.ArraySize;
			m_ResourcesDescriptionTable[name] = resourceDesc;

			stats[texture.Set].StorageTextures++;
		}
		for (const auto& [name, sampler] : m_MetaData.Samplers)
		{
			VkDescriptorSetLayoutBinding layoutBinding = {};
			layoutBinding.binding = sampler.Binding;
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			layoutBinding.descriptorCount = sampler.ArraySize;
			layoutBinding.stageFlags = Vulkan::GetShaderStageFlags(sampler.StageFlags);
			layoutBinding.pImmutableSamplers = nullptr;
			bindings[sampler.Set].push_back(layoutBinding);

			ShaderResourceDescription resourceDesc;
			resourceDesc.Type = ShaderResourceType::Sampler;
			resourceDesc.Binding = sampler.Binding;
			resourceDesc.Set = sampler.Set;
			resourceDesc.ArraySize = sampler.ArraySize;
			m_ResourcesDescriptionTable[name] = resourceDesc;

			stats[sampler.Set].Samplers++;
		}
		for (const auto& [name, buffer] : m_MetaData.UniformBuffers)
		{
			VkDescriptorSetLayoutBinding layoutBinding = {};
			layoutBinding.binding = buffer.Binding;
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			layoutBinding.descriptorCount = buffer.ArraySize;
			layoutBinding.stageFlags = Vulkan::GetShaderStageFlags(buffer.StageFlags);
			layoutBinding.pImmutableSamplers = nullptr;
			bindings[buffer.Set].push_back(layoutBinding);

			ShaderResourceDescription resourceDesc;
			resourceDesc.Type = ShaderResourceType::UniformBuffer;
			resourceDesc.Binding = buffer.Binding;
			resourceDesc.Set = buffer.Set;
			resourceDesc.ArraySize = buffer.ArraySize;
			m_ResourcesDescriptionTable[name] = resourceDesc;

			stats[buffer.Set].UBOs++;
		}
		for (const auto& [name, buffer] : m_MetaData.StorageBuffers)
		{
			VkDescriptorSetLayoutBinding layoutBinding = {};
			layoutBinding.binding = buffer.Binding;
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			layoutBinding.descriptorCount = buffer.ArraySize;
			layoutBinding.stageFlags = Vulkan::GetShaderStageFlags(buffer.StageFlags);
			layoutBinding.pImmutableSamplers = nullptr;
			bindings[buffer.Set].push_back(layoutBinding);

			ShaderResourceDescription resourceDesc;
			resourceDesc.Type = ShaderResourceType::StorageBuffer;
			resourceDesc.Binding = buffer.Binding;
			resourceDesc.Set = buffer.Set;
			resourceDesc.ArraySize = buffer.ArraySize;
			m_ResourcesDescriptionTable[name] = resourceDesc;

			stats[buffer.Set].SBOs++;
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
			ATN_CORE_INFO_TAG("Renderer", "Create descriptor set layout {} with {} textures, {} storage textures, {} separate samplers, {} ubos, {} sbos", 
				set, setStats.SampledTextures, setStats.StorageTextures, setStats.Samplers, setStats.UBOs, setStats.SBOs);
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = m_DescriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = m_DescriptorSetLayouts.data();

		VkPushConstantRange range = {};
		const auto& pushConstant = m_MetaData.PushConstant;

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
		Vulkan::SetObjectDebugName(m_PipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, std::format("{}Layout", m_Name));
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
			Vulkan::SetObjectDebugName(m_VulkanShaderModules[stage], VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, std::format("{}_{}", m_Name, debugNames[stage]));

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

			if(pipelineLayout)
				vkDestroyPipelineLayout(VulkanContext::GetLogicalDevice(), pipelineLayout, nullptr);
		});

		m_PipelineShaderStages.clear();
		m_DescriptorSetLayouts.clear();
		m_VulkanShaderModules.clear();
		m_PipelineLayout = VK_NULL_HANDLE;
	}
}
