#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/ShaderCompiler.h"
#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"
#include "Athena/Renderer/RHI/Shared/DXC.h"


namespace Athena
{
	class D3D12ShaderCompiler : public ShaderCompiler
	{
	public:
		D3D12ShaderCompiler() = default;
		virtual ~D3D12ShaderCompiler() = default;

		virtual void RHISetupEnvironment(ShaderModel shaderModel, ShaderCompilerEnvironment& environment) override;
		virtual ShaderPreprocessResult Preprocess(const String& source) override;
		virtual ShaderCompilationResult Compile(const String& source) override;

	private:
		bool GenerateReflectionData(ShaderReflectionData& data, IDxcBlob* reflectionBlob);
		std::vector<std::wstring> GetCompilerArgsFromEnv(const ShaderCompilerEnvironment& env, bool preprocessOnly) const;

	private:
		DXC::Compiler m_DXCCompiler;
	};


	class D3D12ShaderInstance : public ShaderInstance
	{
	public:
		D3D12ShaderInstance(const Buffer& bytecode, const ShaderReflectionData& reflectionData);
		virtual ~D3D12ShaderInstance() = default;

		const Buffer& GetDXILBytecode() { return m_DXILBytecode; }

	private:
		Buffer m_DXILBytecode;
	};
}
