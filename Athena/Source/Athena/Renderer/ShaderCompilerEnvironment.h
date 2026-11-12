#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Hash.h"

#include <map>


namespace Athena
{
	enum class ShaderTarget
	{
		D3D12_SM6_0,
		D3D12_SM6_1,
		D3D12_SM6_2,
		D3D12_SM6_3,
		D3D12_SM6_4,
		D3D12_SM6_5,
		D3D12_SM6_6,
		D3D12_SM6_7,
		Vulkan_Spirv_1_3,
		Vulkan_Spirv_1_4,
		Vulkan_Spirv_1_5,
		Vulkan_Spirv_1_6,
	};

	enum class ShaderType
	{
		VertexShader,
		GeometryShader,
		PixelShader,
		ComputeShader
	};

	enum class ShaderOptimizatinLevel
	{
		NoOptimizations,
		Level_1,
		Level_2,
		Level_3
	};

	enum class ShaderMatrixLayout
	{
		ColumnMajor,
		RowMajor
	};

	struct ShaderCompilerEnvironment
	{
		String Name;
		ShaderTarget Target;
		ShaderType ShaderStage;
		String EntryPoint;

		std::vector<FilePath> IncludeDirectories;
		std::map<String, int64> Defines; // Use map to garantuee order

		ShaderOptimizatinLevel OptimizationLevel = ShaderOptimizatinLevel::Level_3;
		ShaderMatrixLayout MatrixLayout = ShaderMatrixLayout::RowMajor;
		bool Enable16BitTypes = false;
		bool GenerateDebugInfo = false;
		bool TreatWarningsAsErrors = true;

		void AddDefine(const String& name, int64 value) 
		{
			ensuref(!Defines.contains(name));
			Defines[name] = value;
		}

		void AddIncludeDirectory(const FilePath& path)
		{
			IncludeDirectories.push_back(path);
		}

		uint64 GetHash() const
		{
			uint64 hash = 0;
			HashCombine(hash, (uint32)Target);
			HashCombine(hash, (uint32)ShaderStage);
			HashCombine(hash, EntryPoint);
			HashCombine(hash, (uint32)OptimizationLevel);
			HashCombine(hash, (uint32)MatrixLayout);
			HashCombine(hash, Enable16BitTypes);
			HashCombine(hash, GenerateDebugInfo);
			HashCombine(hash, TreatWarningsAsErrors);

			for (const auto& includeDir : IncludeDirectories)
			{
				HashCombine(hash, includeDir);
			}

			for (const auto& [name, value] : Defines)
			{
				HashCombine(hash, name);
				HashCombine(hash, value);
			}

			return hash;
		}
	};

	inline const char* GetShaderTargetFileExtension(ShaderTarget target)
	{
		switch (target)
		{
		case ShaderTarget::D3D12_SM6_0:
		case ShaderTarget::D3D12_SM6_1:
		case ShaderTarget::D3D12_SM6_2:
		case ShaderTarget::D3D12_SM6_3:
		case ShaderTarget::D3D12_SM6_4:
		case ShaderTarget::D3D12_SM6_5:
		case ShaderTarget::D3D12_SM6_6:
		case ShaderTarget::D3D12_SM6_7:
			return "dxil";

		case ShaderTarget::Vulkan_Spirv_1_3:
		case ShaderTarget::Vulkan_Spirv_1_4:
		case ShaderTarget::Vulkan_Spirv_1_5:
		case ShaderTarget::Vulkan_Spirv_1_6:
			return "spv";
		}

		checkf(false);
		return "";
	}

	namespace EnumUtils
	{
		inline const char* ShaderTargetToString(ShaderTarget target)
		{
			switch (target)
			{
			case ShaderTarget::D3D12_SM6_0: return "D3D12_SM6_0";
			case ShaderTarget::D3D12_SM6_1: return "D3D12_SM6_1";
			case ShaderTarget::D3D12_SM6_2: return "D3D12_SM6_2";
			case ShaderTarget::D3D12_SM6_3: return "D3D12_SM6_3";
			case ShaderTarget::D3D12_SM6_4: return "D3D12_SM6_4";
			case ShaderTarget::D3D12_SM6_5: return "D3D12_SM6_5";
			case ShaderTarget::D3D12_SM6_6: return "D3D12_SM6_6";
			case ShaderTarget::D3D12_SM6_7: return "D3D12_SM6_7";

			case ShaderTarget::Vulkan_Spirv_1_3: return "Vulkan_Spirv_1_3";
			case ShaderTarget::Vulkan_Spirv_1_4: return "Vulkan_Spirv_1_4";
			case ShaderTarget::Vulkan_Spirv_1_5: return "Vulkan_Spirv_1_5";
			case ShaderTarget::Vulkan_Spirv_1_6: return "Vulkan_Spirv_1_6";
			}

			checkf(false);
			return "";
		}

		inline std::string_view ShaderTypeToString(ShaderType type)
		{
			switch (type)
			{
			case ShaderType::VertexShader: return "VertexShader";
			case ShaderType::PixelShader: return "PixelShader";
			case ShaderType::GeometryShader: return "GeometryShader";
			case ShaderType::ComputeShader: return "ComputeShader";
			}

			checkf(false);
			return "";
		}
	}

}
