#include "ShaderCompiler.h"
#include "Athena/Core/ConsoleManager.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	static AutoCVar<bool> CVarShaderGenerateDebugInfo(
		"r.Shader.GenerateDebugInfo",
		true,
		"Sets globally if shaders compilation generates debug info."
	);

	static AutoCVar<int32> CVarShaderOptimizationLevel(
		"r.Shader.OptimizationLevel",
		3,
		"Sets globally shaders optimization level (0 - no optimizations, 3 - highest)."
	);

	static AutoCVar<bool> CVarShaderTreatWarningsAsErrors(
		"r.Shader.TreatWarningsAsErrors",
		true,
		"Sets globally if warning are teated as errors during shader compilation."
	);


	Ref<ShaderCompiler> ShaderCompiler::Create()
	{
		return RHI::Get()->CreateShaderCompiler();
	}

	ShaderCompiler::ShaderCompiler()
	{
		
	}

	const ShaderCompilerEnvironment& ShaderCompiler::SetupEnvironment(ShaderModel shaderModel, const ShaderDefinition* def, const PermutationVector& vector, const String& name)
	{
		m_Environment = {};

		ShaderOptimizatinLevel olevel = (ShaderOptimizatinLevel)Math::Clamp(CVarShaderOptimizationLevel.GetInt(), ShaderOptimizatinLevel::NoOptimizations, ShaderOptimizatinLevel::Level_3);

		m_Environment.Name = name;
		m_Environment.ShaderStage = def->GetType();
		m_Environment.EntryPoint = def->GetEntryPoint();
		m_Environment.OptimizationLevel = olevel;
		m_Environment.MatrixLayout = ShaderMatrixLayout::RowMajor;
		m_Environment.Enable16BitTypes = false;
		m_Environment.GenerateDebugInfo = CVarShaderGenerateDebugInfo.GetBool();
		m_Environment.TreatWarningsAsErrors = CVarShaderTreatWarningsAsErrors.GetBool();

		for (const auto& [name, value] : vector.Vector)
		{
			m_Environment.AddDefine(name, value);
		}

		m_Environment.AddDefine("ATN_PLATFORM_WINDOWS", ATN_PLATFORM_WINDOWS);

		if (def->GetRelativeFilePath().has_parent_path())
		{
			m_Environment.AddIncludeDirectory(def->GetRelativeFilePath().parent_path());
		}

		m_Environment.AddIncludeDirectory(ShaderManager::Get().GetShaderSourcesPath());

		def->GetShader()->ModifyCompilerEnvironment(vector, m_Environment);
		RHISetupEnvironment(shaderModel, m_Environment);

		return m_Environment;
	}
}
