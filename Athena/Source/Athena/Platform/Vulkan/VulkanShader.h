#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/ShaderCompiler.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanShader : public Shader
	{
	public:
		VulkanShader(const FilePath& path, const String& name);
		VulkanShader(const FilePath& path);

		~VulkanShader();

		virtual void Reload() override;

		const std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineStages() const { return m_PipelineShaderStages; }
		std::vector<VkDescriptorSetLayout> GetAllDescriptorSetLayouts() const { return m_DescriptorSetLayouts; }
		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

	private:
		void CompileOrGetFromCache(bool forceCompile);
		void CreateVulkanShaderModulesAndStages(const ShaderCompiler& compiler);
		void CleanUp();

	private:
		std::unordered_map<ShaderStage, VkShaderModule> m_VulkanShaderModules;
		std::vector<VkPipelineShaderStageCreateInfo> m_PipelineShaderStages;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
		VkPipelineLayout m_PipelineLayout;
	};
}
