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

	private:
		bool CompileOrGetBinaries(bool forceCompile = false);
		bool CheckShaderStages(const std::unordered_map<ShaderType, String>& sources);
		void Reflect(ShaderType type, const std::vector<uint32>& src);

		void CreateVulkanShaderModules();
		void CleanUp();

	private:
		bool m_Compiled = false;
		std::unordered_map<ShaderType, std::vector<uint32>> m_SPIRVBinaries;
		std::unordered_map<ShaderType, VkShaderModule> m_VulkanShaderModules;
	};
}
