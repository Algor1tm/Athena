#include "D3D12Shader.h"

#include <d3d12shader.h>


namespace Athena
{
	namespace D3D12
	{
		static D3D_SHADER_MODEL TranslateShaderModel(ShaderModel model)
		{
			switch (model)
			{
			case ShaderModel::SM_6_0: return D3D_SHADER_MODEL_6_0;
			case ShaderModel::SM_6_1: return D3D_SHADER_MODEL_6_1;
			case ShaderModel::SM_6_2: return D3D_SHADER_MODEL_6_2;
			case ShaderModel::SM_6_3: return D3D_SHADER_MODEL_6_3;
			case ShaderModel::SM_6_4: return D3D_SHADER_MODEL_6_4;
			case ShaderModel::SM_6_5: return D3D_SHADER_MODEL_6_5;
			case ShaderModel::SM_6_6: return D3D_SHADER_MODEL_6_6;
			case ShaderModel::SM_6_7: return D3D_SHADER_MODEL_6_7;
			}
			
			checkf(false);
			return D3D_SHADER_MODEL_5_1;
		}

		static ShaderTarget TranslateShaderTarget(D3D_SHADER_MODEL model)
		{
			switch (model)
			{
			case D3D_SHADER_MODEL_6_0: return ShaderTarget::D3D12_SM6_0;
			case D3D_SHADER_MODEL_6_1: return ShaderTarget::D3D12_SM6_1;
			case D3D_SHADER_MODEL_6_2: return ShaderTarget::D3D12_SM6_2;
			case D3D_SHADER_MODEL_6_3: return ShaderTarget::D3D12_SM6_3;
			case D3D_SHADER_MODEL_6_4: return ShaderTarget::D3D12_SM6_4;
			case D3D_SHADER_MODEL_6_5: return ShaderTarget::D3D12_SM6_5;
			case D3D_SHADER_MODEL_6_6: return ShaderTarget::D3D12_SM6_6;
			case D3D_SHADER_MODEL_6_7: return ShaderTarget::D3D12_SM6_7;
			}

			checkf(false);
			return ShaderTarget::D3D12_SM6_0;
		}

		static TextureType TranslateSRVDimension(D3D_SRV_DIMENSION dim)
		{
			switch (dim)
			{
			case D3D_SRV_DIMENSION_TEXTURE2D:
			case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
				return TextureType::TEXTURE_2D;
			case D3D_SRV_DIMENSION_TEXTURECUBE:
			case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
				return TextureType::TEXTURE_CUBE;
			}

			checkf(false);
			return TextureType::TEXTURE_2D;
		}
	}

	void D3D12ShaderCompiler::RHISetupEnvironment(ShaderModel shaderModel, ShaderCompilerEnvironment& environment)
	{
		D3D_SHADER_MODEL supportedShaderModel = D3D12RHI::Get()->GetFeaturesInfo().ShaderModel;
		D3D_SHADER_MODEL requestedShaderModel = D3D12::TranslateShaderModel(shaderModel);

		if (supportedShaderModel < requestedShaderModel)
		{
			ATN_LOG_WARN(D3D12, "D3D12 device does not supported requested shader model, falling back to supported version.");
			requestedShaderModel = supportedShaderModel;
		}

		environment.Target = D3D12::TranslateShaderTarget(requestedShaderModel);
		environment.AddDefine("ATN_API_VULKAN", 0);
		environment.AddDefine("ATN_API_D3D12", 1);

		m_DXCCompiler.SetIncludeDirectories(environment.IncludeDirectories);
		m_DXCCompiler.SetDebugName(environment.Name);
	}

	ShaderPreprocessResult D3D12ShaderCompiler::Preprocess(const String& source)
	{
		ShaderPreprocessResult preprocessResult;

		std::vector<std::wstring> args = GetCompilerArgsFromEnv(GetEnvironment(), true);
		preprocessResult.Success = m_DXCCompiler.Preprocess(source, args, &preprocessResult.PreprocessedSource);

		return preprocessResult;
	}

	ShaderCompilationResult D3D12ShaderCompiler::Compile(const String& source)
	{
		ShaderCompilationResult compilationResult;

		std::vector<std::wstring> args = GetCompilerArgsFromEnv(GetEnvironment(), false);

		IDxcBlob* reflectionBlob;
		compilationResult.Success = m_DXCCompiler.Compile(source, args, &reflectionBlob, &compilationResult.Bytecode);

		if (compilationResult.Success)
		{
			compilationResult.Success = GenerateReflectionData(compilationResult.ReflectionData, reflectionBlob);
			reflectionBlob->Release();
		}

		return compilationResult;
	}

	bool D3D12ShaderCompiler::GenerateReflectionData(ShaderReflectionData& data, IDxcBlob* reflectionBlob)
	{
		DxcBuffer reflectionBuffer = {};
		reflectionBuffer.Ptr = reflectionBlob->GetBufferPointer();
		reflectionBuffer.Size = reflectionBlob->GetBufferSize();
		reflectionBuffer.Encoding = 0;

		ID3D12ShaderReflection* reflection;
		HRESULT hr = m_DXCCompiler.GetUtils()->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflection));

		if (FAILED(hr))
		{
			ATN_LOG_ERROR(D3D12, "Failed to generate reflection data for shader '{}'", GetEnvironment().Name);
			ensuref(false);
			return false;
		}

		D3D12_SHADER_DESC shaderDesc = {};
		reflection->GetDesc(&shaderDesc);

		data.InputsCount = shaderDesc.InputParameters;
		data.OutputsCount = shaderDesc.OutputParameters;

		UINT x, y, z;
		UINT threadCount = reflection->GetThreadGroupSize(&x, &y, &z);
		data.WorkGroupSize = threadCount > 0 ? Vector3u(x, y, z) : Vector3u(0, 0, 0);

		for (uint32 i = 0; i < shaderDesc.BoundResources; ++i)
		{
			D3D12_SHADER_INPUT_BIND_DESC desc = {};
			reflection->GetResourceBindingDesc(i, &desc);

			if (desc.uFlags & D3D_SIF_UNUSED)
				continue;

			const char* name = desc.Name;
			D3D_SHADER_INPUT_TYPE type = desc.Type;

			ResourceBindingData binding;
			binding.BindPoint = desc.BindPoint;
			binding.Set = desc.Space;
			binding.ArraySize = desc.BindCount;

			switch (type)
			{
			case D3D_SIT_TEXTURE:
			{
				TextureReflectionData& res = data.Textures[name];
				res.TextureType = D3D12::TranslateSRVDimension(desc.Dimension);
				res.Binding = binding;
				res.ReadOnly = true;
				break;
			}
			case D3D_SIT_UAV_RWTYPED:
			{
				TextureReflectionData& res = data.Textures[name];
				res.TextureType = D3D12::TranslateSRVDimension(desc.Dimension);
				res.Binding = binding;
				res.ReadOnly = false;
				break;
			}
			case D3D_SIT_SAMPLER:
			{
				SamplerReflectionData& res = data.Samplers[name];
				res.Binding = binding;
				break;
			}
			case D3D_SIT_STRUCTURED:
			{
				BufferReflectionData& res = data.StructuredBuffers[name];
				res.Binding = binding;
				res.ReadOnly = true;
				break;
			}
			case D3D_SIT_UAV_RWSTRUCTURED:
			{
				BufferReflectionData& res = data.StructuredBuffers[name];
				res.Binding = binding;
				res.ReadOnly = false;
				break;
			}
			case D3D_SIT_BYTEADDRESS:
			{
				BufferReflectionData& res = data.ByteAddressBuffers[name];
				res.Binding = binding;
				res.ReadOnly = true;
				break;
			}
			case D3D_SIT_UAV_RWBYTEADDRESS:
			{
				BufferReflectionData& res = data.ByteAddressBuffers[name];
				res.Binding = binding;
				res.ReadOnly = false;
				break;
			}
			case D3D_SIT_CBUFFER:
			{
				UniformBufferReflectionData& res = data.UniformBuffers[name];
				res.Binding = binding;

				ID3D12ShaderReflectionConstantBuffer* buffer = reflection->GetConstantBufferByIndex(i);
				D3D12_SHADER_BUFFER_DESC bufferDesc = {}; 
				buffer->GetDesc(&bufferDesc);

				res.Size = bufferDesc.Size;

				break;
			}
			}
		}

		reflection->Release();
		return true;
	}

	std::vector<std::wstring> D3D12ShaderCompiler::GetCompilerArgsFromEnv(const ShaderCompilerEnvironment& env, bool preprocessOnly) const
	{
		std::vector<std::wstring> args;

		auto Add = [&](std::wstring argument)
			{
				args.push_back(std::move(argument));
			};

		auto AddOption = [&](const wchar_t* option, std::wstring value)
			{
				Add(option);
				Add(std::move(value));
			};

		for (const auto& path : env.IncludeDirectories)
		{
			AddOption(L"-I", path.wstring());
		}

		for (const auto& [name, value] : env.Defines)
		{
			String str = fmt::format("{}={}", name, value);
			AddOption(L"-D", FilePath(str).wstring());
		}

		if (preprocessOnly)
		{
			Add(L"-P");
			return args;
		}

		// dont embedd reflection in dxil as we store it separately
		Add(L"-Qstrip_reflect");

		const wchar_t* stage = nullptr;

		switch (env.ShaderStage)
		{
		case ShaderType::VertexShader:   stage = L"vs"; break;
		case ShaderType::GeometryShader: stage = L"gs"; break;
		case ShaderType::PixelShader:    stage = L"ps"; break;
		case ShaderType::ComputeShader:  stage = L"cs"; break;
		}

		const wchar_t* profile;

		switch (env.Target)
		{
		case ShaderTarget::D3D12_SM6_0: profile = L"6_0"; break;
		case ShaderTarget::D3D12_SM6_1: profile = L"6_1"; break;
		case ShaderTarget::D3D12_SM6_2: profile = L"6_2"; break;
		case ShaderTarget::D3D12_SM6_3: profile = L"6_3"; break;
		case ShaderTarget::D3D12_SM6_4: profile = L"6_4"; break;
		case ShaderTarget::D3D12_SM6_5: profile = L"6_5"; break;
		case ShaderTarget::D3D12_SM6_6: profile = L"6_6"; break;
		case ShaderTarget::D3D12_SM6_7: profile = L"6_7"; break;
		}

		std::wstring target = stage;
		target += L"_";
		target += profile;

		AddOption(L"-T", std::move(target));
		AddOption(L"-E", FilePath(env.EntryPoint).wstring());

		switch (env.OptimizationLevel)
		{
		case ShaderOptimizatinLevel::NoOptimizations: Add(L"-O0"); break;
		case ShaderOptimizatinLevel::Level_1:         Add(L"-O1"); break;
		case ShaderOptimizatinLevel::Level_2:         Add(L"-O2"); break;
		case ShaderOptimizatinLevel::Level_3:         Add(L"-O3"); break;
		}

		switch (env.MatrixLayout)
		{
		case ShaderMatrixLayout::ColumnMajor: Add(L"-Zpc"); break;
		case ShaderMatrixLayout::RowMajor:    Add(L"-Zpr"); break;
		}

		if (env.Enable16BitTypes)
		{
			bool supported = D3D12RHI::Get()->GetFeaturesInfo().Options4.Native16BitShaderOpsSupported;

			if(supported)
				Add(L"-enable-16bit-types");
			else
				ATN_LOG_WARN(D3D12, "D3D12 device does not support 16-bit types in shaders, skipping Enable16BitTypes option!");
		}

		if (env.GenerateDebugInfo)
		{
			Add(L"-Zi");
			Add(L"-Qembed_debug");
		}

		if (env.TreatWarningsAsErrors)
			Add(L"-WX");

		return args;
	}

	D3D12ShaderInstance::D3D12ShaderInstance(const Buffer& bytecode, const ShaderReflectionData& reflectionData)
		: ShaderInstance(reflectionData)
		, m_DXILBytecode(Buffer::Copy(bytecode))
	{

	}
}
