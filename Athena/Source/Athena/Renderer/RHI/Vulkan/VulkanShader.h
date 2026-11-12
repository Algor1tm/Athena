#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/ShaderCompiler.h"
#include "Athena/Renderer/RHI/Shared/DXC.h"

#include <vulkan/vulkan.h>


namespace Athena
{
	class VulkanShaderCompiler : public ShaderCompiler
	{
	public:
		VulkanShaderCompiler() = default;
		virtual ~VulkanShaderCompiler() = default;

		virtual void RHISetupEnvironment(ShaderModel shaderModel, ShaderCompilerEnvironment& environment) override;
		virtual ShaderPreprocessResult Preprocess(const String& source) override;
		virtual ShaderCompilationResult Compile(const String& source) override;

	private:
		bool GenerateReflectionData(ShaderReflectionData& data, const Buffer& spirv);
		std::vector<std::wstring> GetCompilerArgsFromEnv(const ShaderCompilerEnvironment& env, bool preprocessOnly) const;

	private:
		DXC::Compiler m_DXCCompiler;
	};


	class VulkanShaderInstance : public ShaderInstance
	{
	public:
		VulkanShaderInstance(const Buffer& bytecode, const ShaderReflectionData& reflectionData);
		virtual ~VulkanShaderInstance();

		VkShaderModule GetShaderModel() const { return m_ShaderModule; }

	private:
		VkShaderModule m_ShaderModule;
	};
}
