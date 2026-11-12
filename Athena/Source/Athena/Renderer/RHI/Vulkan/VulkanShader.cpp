#include "VulkanShader.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"

#include <SPIRV-Reflect/spirv_reflect.h>


namespace Athena
{
	namespace Vulkan
	{
		static ShaderTarget GetSupportedShaderTarget(uint32 vulkanVersion)
		{
			switch (VK_API_VERSION_MAJOR(vulkanVersion) * 10 + VK_API_VERSION_MINOR(vulkanVersion))
			{
			case 11: return ShaderTarget::Vulkan_Spirv_1_3;
			case 12: return ShaderTarget::Vulkan_Spirv_1_5;
			case 13: return ShaderTarget::Vulkan_Spirv_1_6;
			case 14: return ShaderTarget::Vulkan_Spirv_1_6;
			}

			checkf(false);
			return ShaderTarget::Vulkan_Spirv_1_3;
		}

		static ShaderTarget ShaderModelToSpirvVersion(ShaderModel shaderModel)
		{
			switch (shaderModel)
			{
			case ShaderModel::SM_6_0: 
			case ShaderModel::SM_6_1:
			case ShaderModel::SM_6_2:
				return ShaderTarget::Vulkan_Spirv_1_3;

			case ShaderModel::SM_6_3:
			case ShaderModel::SM_6_4:
				return ShaderTarget::Vulkan_Spirv_1_4;

			case ShaderModel::SM_6_5:
				return ShaderTarget::Vulkan_Spirv_1_5;

			case ShaderModel::SM_6_6:
			case ShaderModel::SM_6_7:
				return ShaderTarget::Vulkan_Spirv_1_6;
			}

			checkf(false);
			return ShaderTarget::Vulkan_Spirv_1_3;
		}

		static TextureType TranslateSpirvDim(SpvDim dim)
		{
			switch (dim)
			{
			case SpvDim2D: return TextureType::TEXTURE_2D;
			case SpvDimCube: return TextureType::TEXTURE_CUBE;
			}

			//checkf(false);
			return TextureType::TEXTURE_2D;
		}
	}

	void VulkanShaderCompiler::RHISetupEnvironment(ShaderModel shaderModel, ShaderCompilerEnvironment& environment)
	{
		uint32 deviceVulkanVersion = VulkanRHI::Get()->GetDevice()->GetVulkanVersion();

		ShaderTarget supportedShaderTarget = Vulkan::GetSupportedShaderTarget(deviceVulkanVersion);
		ShaderTarget shaderTarget = Vulkan::ShaderModelToSpirvVersion(shaderModel);

		if (supportedShaderTarget < shaderTarget)
		{
			ATN_LOG_WARN(Vulkan, "Vulkan device does not supported requested shader model, falling back to supported version.");
			shaderTarget = supportedShaderTarget;
		}

		environment.Target = shaderTarget;
		environment.AddDefine("ATN_API_VULKAN", 1);
		environment.AddDefine("ATN_API_D3D12", 0);

		m_DXCCompiler.SetIncludeDirectories(environment.IncludeDirectories);
		m_DXCCompiler.SetDebugName(environment.Name);
	}

	ShaderPreprocessResult VulkanShaderCompiler::Preprocess(const String& source)
	{
		ShaderPreprocessResult preprocessResult;

		std::vector<std::wstring> args = GetCompilerArgsFromEnv(GetEnvironment(), true);
		preprocessResult.Success = m_DXCCompiler.Preprocess(source, args, &preprocessResult.PreprocessedSource);

		return preprocessResult;
	}

	ShaderCompilationResult VulkanShaderCompiler::Compile(const String& source)
	{
		ShaderCompilationResult compilationResult;

		std::vector<std::wstring> args = GetCompilerArgsFromEnv(GetEnvironment(), false);
		compilationResult.Success = m_DXCCompiler.Compile(source, args, nullptr, &compilationResult.Bytecode);

		if (compilationResult.Success)
		{
			compilationResult.Success = GenerateReflectionData(compilationResult.ReflectionData, compilationResult.Bytecode);
		}

		return compilationResult;
	}

	bool VulkanShaderCompiler::GenerateReflectionData(ShaderReflectionData& data, const Buffer& spirv)
	{
		SpvReflectShaderModule module = {};
		SpvReflectResult result = spvReflectCreateShaderModule(spirv.Size(), spirv.Data(), &module);

		if (result != SPV_REFLECT_RESULT_SUCCESS)
		{
			ATN_LOG_ERROR(Vulkan, "Failed to generate reflection data for shader '{}'", GetEnvironment().Name);
			ensuref(false);
			return false;
		}

		const SpvReflectEntryPoint* entryPoint = spvReflectGetEntryPoint(&module, GetEnvironment().EntryPoint.c_str());

		if (entryPoint)
		{
			data.WorkGroupSize.x = entryPoint->local_size.x;
			data.WorkGroupSize.y = entryPoint->local_size.y;
			data.WorkGroupSize.z = entryPoint->local_size.z;
		}

		spvReflectEnumerateInputVariables(&module, &data.InputsCount, nullptr);
		spvReflectEnumerateOutputVariables(&module, &data.OutputsCount, nullptr);

		uint32_t resourcesCount = 0;
		spvReflectEnumerateDescriptorBindings(&module, &resourcesCount, nullptr);

		std::vector<SpvReflectDescriptorBinding*> resources(resourcesCount);
		spvReflectEnumerateDescriptorBindings(&module, &resourcesCount, resources.data());

		for (const SpvReflectDescriptorBinding* descriptor : resources)
		{
			if (!descriptor->accessed)
				continue;

			ResourceBindingData binding;
			binding.BindPoint = descriptor->binding;
			binding.Set = descriptor->set;
			binding.ArraySize = descriptor->count;

			switch (descriptor->descriptor_type)
			{
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
			{
				SamplerReflectionData& res = data.Samplers[descriptor->name];
				res.Binding = binding;
				break;
			}
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			{
				TextureReflectionData& res = data.Textures[descriptor->name];
				res.TextureType = Vulkan::TranslateSpirvDim(descriptor->image.dim);
				res.Binding = binding;
				res.ReadOnly = true;
				break;
			}
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			{
				TextureReflectionData& res = data.Textures[descriptor->name];
				res.TextureType = Vulkan::TranslateSpirvDim(descriptor->image.dim);
				res.Binding = binding;
				res.ReadOnly = false;
				break;
			}
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
			{
				UniformBufferReflectionData& res = data.UniformBuffers[descriptor->name];
				res.Size = descriptor->block.size;
				res.Binding = binding;
				break;
			}
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			{
				switch (descriptor->user_type)
				{
				case SPV_REFLECT_USER_TYPE_STRUCTURED_BUFFER:
				{
					BufferReflectionData& res = data.StructuredBuffers[descriptor->name];
					res.Binding = binding;
					res.ReadOnly = true;
					break;
				}
				case SPV_REFLECT_USER_TYPE_RW_STRUCTURED_BUFFER:
				{
					BufferReflectionData& res = data.StructuredBuffers[descriptor->name];
					res.Binding = binding;
					res.ReadOnly = false;
					break;
				}
				case SPV_REFLECT_USER_TYPE_BYTE_ADDRESS_BUFFER:
				{
					BufferReflectionData& res = data.ByteAddressBuffers[descriptor->name];
					res.Binding = binding;
					res.ReadOnly = true;
					break;
				}
				case SPV_REFLECT_USER_TYPE_RW_BYTE_ADDRESS_BUFFER:
				{
					BufferReflectionData& res = data.ByteAddressBuffers[descriptor->name];
					res.Binding = binding;
					res.ReadOnly = false;
					break;
				}
				}
				break;
			}
			}
		}

		spvReflectDestroyShaderModule(&module);
		return true;
	}

	std::vector<std::wstring> VulkanShaderCompiler::GetCompilerArgsFromEnv(const ShaderCompilerEnvironment& env, bool preprocessOnly) const
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

		Add(L"-spirv");

		const wchar_t* vulkanVersion = nullptr;
		switch (env.Target)
		{
		case ShaderTarget::Vulkan_Spirv_1_3: vulkanVersion = L"1.1"; break;
		case ShaderTarget::Vulkan_Spirv_1_4: vulkanVersion = L"1.2"; break;
		case ShaderTarget::Vulkan_Spirv_1_5: vulkanVersion = L"1.2"; break;
		case ShaderTarget::Vulkan_Spirv_1_6: vulkanVersion = L"1.3"; break;
		}

		std::wstring vulkanEnv = L"-fspv-target-env=vulkan";
		vulkanEnv += vulkanVersion;
		Add(vulkanEnv);

		Add(L"-fvk-use-dx-layout");

		if (env.ShaderStage == ShaderType::VertexShader || env.ShaderStage == ShaderType::GeometryShader)
			Add(L"-fvk-invert-y");

		if (preprocessOnly)
		{
			Add(L"-P");
			return args;
		}

		Add(L"-fspv-reflect");

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
		case ShaderTarget::Vulkan_Spirv_1_3: profile = L"6_2"; break;
		case ShaderTarget::Vulkan_Spirv_1_4: profile = L"6_4"; break;
		case ShaderTarget::Vulkan_Spirv_1_5: profile = L"6_5"; break;
		case ShaderTarget::Vulkan_Spirv_1_6: profile = L"6_7"; break;
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
			const DeviceFeaturesInfo& info = VulkanRHI::Get()->GetDevice()->GetFeaturesInfo();

			if (info.Float16Int8Features.shaderFloat16 && info.ShaderInt16)
				Add(L"-enable-16bit-types");
			else
				ATN_LOG_WARN(Vulkan, "Vulkan device does not support 16-bit types in shaders, skipping Enable16BitTypes option!");
		}

		if (env.GenerateDebugInfo)
		{
			Add(L"-fspv-debug=source");

			// Does not compile with this option
			//Add(L"-fspv-debug=vulkan-with-source");
		}

		if (env.TreatWarningsAsErrors)
			Add(L"-WX");

		return args;
	}

	VulkanShaderInstance::VulkanShaderInstance(const Buffer& spirv, const ShaderReflectionData& reflectionData)
		: ShaderInstance(reflectionData)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = spirv.Size();
		createInfo.pCode = (uint32_t*)spirv.Data();

		VK_CHECK(vkCreateShaderModule(VulkanRHI::Get()->GetDevice()->GetLogicalDevice(), &createInfo, nullptr, &m_ShaderModule));
	}

	VulkanShaderInstance::~VulkanShaderInstance()
	{
		vkDestroyShaderModule(VulkanRHI::Get()->GetDevice()->GetLogicalDevice(), m_ShaderModule, nullptr);
	}
}