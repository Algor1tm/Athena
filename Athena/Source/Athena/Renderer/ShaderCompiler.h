#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Renderer/RenderThread.h"
#include "Shader.h"


namespace Athena
{
	struct ShaderPreprocessResult
	{
		bool Success;
		String PreprocessedSource;
	};

	struct ShaderCompilationResult
	{
		bool Success;
		Buffer Bytecode;
		ShaderReflectionData ReflectionData;
	};


	class ATHENA_API ShaderCompiler: public RefCounted
	{
	public:
		static Ref<ShaderCompiler> Create();

		ShaderCompiler();
		virtual ~ShaderCompiler() = default;

		const ShaderCompilerEnvironment& SetupEnvironment(ShaderModel shaderModel, const ShaderDefinition* def, const PermutationVector& vector, const String& name);
		const ShaderCompilerEnvironment& GetEnvironment() const { return m_Environment; }

		virtual void RHISetupEnvironment(ShaderModel shaderModel, ShaderCompilerEnvironment& environment) = 0;
		virtual ShaderPreprocessResult Preprocess(const String& source) = 0;
		virtual ShaderCompilationResult Compile(const String& source) = 0;

	private:
		ShaderCompilerEnvironment m_Environment;
	};
}
