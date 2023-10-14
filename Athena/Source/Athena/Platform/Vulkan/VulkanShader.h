#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Shader.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanShader : public Shader
	{
	public:
		VulkanShader(const FilePath& path, const String& name);
		VulkanShader(const FilePath& path);

		~VulkanShader();

		virtual bool IsCompiled() override;
		virtual void Reload() override;

		const std::vector<VkPipelineShaderStageCreateInfo>& GetPipelineStages() const { return m_PipelineShaderStages; }

	private:
		bool PreProcess(const String& source, std::unordered_map<ShaderType, String>& result);

		bool CompileOrGetBinaries(bool forceCompile = false);
		bool CheckShaderStages(const std::unordered_map<ShaderType, String>& sources);
		void Reflect(ShaderType type, const std::vector<uint32>& src);

		void CreateVulkanShaderModulesAndStages();
		void CleanUp();

	private:
		bool m_IsHlsl = true;
		bool m_Compiled = false;
		std::unordered_map<ShaderType, std::vector<uint32>> m_SPIRVBinaries;

		std::unordered_map<ShaderType, VkShaderModule> m_VulkanShaderModules;
		std::vector<VkPipelineShaderStageCreateInfo> m_PipelineShaderStages;
	};
}
