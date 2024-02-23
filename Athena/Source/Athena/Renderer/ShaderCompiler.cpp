#include "ShaderCompiler.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Time.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/GPUBuffers.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>


namespace Athena
{
	namespace Utils
	{
		static FilePath GetCachedFilePath(const FilePath& path, ShaderStage stage)
		{
			FilePath relativeToShaderPack = std::filesystem::relative(path, Renderer::GetShaderPackDirectory());
			FilePath cachedPath = Renderer::GetShaderCacheDirectory() / relativeToShaderPack;

			switch (stage)
			{
			case ShaderStage::VERTEX_STAGE:    cachedPath += L".cached.vert"; break;
			case ShaderStage::FRAGMENT_STAGE:  cachedPath += L".cached.frag"; break;
			case ShaderStage::GEOMETRY_STAGE:  cachedPath += L".cached.geom"; break;
			case ShaderStage::COMPUTE_STAGE:   cachedPath += L".cached.compute"; break;
			default: ATN_CORE_ASSERT(false); return "";
			}

			return cachedPath;
		}

		static shaderc_shader_kind ShaderStageToShaderCKind(ShaderStage stage)
		{
			switch (stage)
			{
			case ShaderStage::VERTEX_STAGE:    return shaderc_glsl_vertex_shader;
			case ShaderStage::FRAGMENT_STAGE:  return shaderc_glsl_fragment_shader;
			case ShaderStage::GEOMETRY_STAGE:  return shaderc_glsl_geometry_shader;
			case ShaderStage::COMPUTE_STAGE:   return shaderc_glsl_compute_shader;
			}

			ATN_CORE_ASSERT(false);
			return (shaderc_shader_kind)0;
		}


		static std::string_view ShaderStageToString(ShaderStage stage)
		{
			switch (stage)
			{
			case ShaderStage::VERTEX_STAGE: return "Vertex Stage";
			case ShaderStage::FRAGMENT_STAGE: return "Fragment Stage";
			case ShaderStage::GEOMETRY_STAGE: return "Geometry Stage";
			case ShaderStage::COMPUTE_STAGE: return "Compute Stage";
			}

			ATN_CORE_ASSERT(false);
			return "";
		}

		static ShaderStage GetShaderStageFromGLSLString(const String& stageString)
		{
			if (stageString == "vertex")
				return ShaderStage::VERTEX_STAGE;

			if (stageString == "fragment")
				return ShaderStage::FRAGMENT_STAGE;

			if (stageString == "geometry")
				return ShaderStage::GEOMETRY_STAGE;

			if (stageString == "compute")
				return ShaderStage::COMPUTE_STAGE;

			return ShaderStage::UNDEFINED;
		}

		static ImageType SpirvDimToImageType(const spv::Dim& dim)
		{
			switch (dim)
			{
			case spv::Dim2D: return ImageType::IMAGE_2D;
			case spv::DimCube: return ImageType::IMAGE_CUBE;
			}

			ATN_CORE_ASSERT(false);
			return ImageType::IMAGE_2D;
		}


		static ShaderDataType SpirvTypeToShaderDataType(const spirv_cross::SPIRType& type)
		{
			// Scalar
			if (type.vecsize == 1 && type.columns == 1)
			{
				switch (type.basetype)
				{
				case spirv_cross::SPIRType::BaseType::Int: return ShaderDataType::Int;
				case spirv_cross::SPIRType::BaseType::Float: return ShaderDataType::Float;
				case spirv_cross::SPIRType::BaseType::UInt: return ShaderDataType::UInt;
				default: return ShaderDataType::Unknown;
				}
			}

			// Vector
			if (type.vecsize != 1 && type.columns == 1)
			{
				switch (type.basetype)
				{
				case spirv_cross::SPIRType::BaseType::Int:
				{
					switch (type.vecsize)
					{
					case 2: return ShaderDataType::Int2;
					case 3: return ShaderDataType::Int3;
					case 4: return ShaderDataType::Int4;
					default: return ShaderDataType::Unknown;
					}
				}
				case spirv_cross::SPIRType::BaseType::Float:
				{
					switch (type.vecsize)
					{
					case 2: return ShaderDataType::Float2;
					case 3: return ShaderDataType::Float3;
					case 4: return ShaderDataType::Float4;
					default: return ShaderDataType::Unknown;
					}
				}
				default: return ShaderDataType::Unknown;
				}
			}

			// Matrix
			if (type.vecsize != 1 && type.columns != 1 && type.vecsize == type.columns)
			{
				switch (type.vecsize)
				{
				case 3: return ShaderDataType::Mat3;
				case 4: return ShaderDataType::Mat4;
				default: return ShaderDataType::Unknown;
				}
			}

			return ShaderDataType::Unknown;
		}
	}

	class FileSystemIncluder : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		virtual shaderc_include_result* GetInclude(
			const char* requested_source,   // relative src path
			shaderc_include_type type,
			const char* requesting_source,	// dst path
			size_t include_depth) override
		{
			FilePath dstParentPath = FilePath(requesting_source).parent_path();
			FilePath srcPath = dstParentPath / requested_source;

			m_SourcePath = srcPath.string();
			m_Source = FileSystem::ReadFile(srcPath);

			shaderc_include_result* result = new shaderc_include_result();

			result->source_name = m_SourcePath.c_str();
			result->source_name_length = m_SourcePath.size();
			result->content = m_Source.c_str();
			result->content_length = m_Source.size();

			return result;
		}

		virtual void ReleaseInclude(shaderc_include_result* data) override
		{
			delete data;
		}

	private:
		String m_SourcePath;
		String m_Source;
	};


	ShaderCompiler::ShaderCompiler(const FilePath& filepath, const String& name)
	{
		m_FilePath = filepath;
		m_Name = name;

		GetLanguageAndEntryPoints();
	}

	std::string_view ShaderCompiler::GetEntryPoint(ShaderStage stage) const
	{
		return m_StageToEntryPointMap.at(stage);
	}

	bool ShaderCompiler::CompileOrGetFromCache(bool forceCompile)
	{
		if (m_Language == ShaderLanguage::NONE)
			return false;

		Timer compilationTimer;

		PreProcessResult result = PreProcess();

		if (!result.ParseResult)
			return false;

		bool compiled = true;
		result.NeedRecompile |= forceCompile;

		if (result.NeedRecompile)
		{
			compiled = CompileAndWriteToCache(result);
		}
		else
		{
			ReadFromCache(result);
		}

		if (result.NeedRecompile)
			ATN_CORE_INFO_TAG("Renderer", "Shader '{}' compilation took {}", m_Name, compilationTimer.ElapsedTime());

		return compiled;
	}

	bool ShaderCompiler::CompileAndWriteToCache(const PreProcessResult& result)
	{
		bool compiled = true;

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		options.SetGenerateDebugInfo();
		options.SetIncluder(std::make_unique<FileSystemIncluder>());

		if (m_Language == ShaderLanguage::GLSL)
		{
			options.SetSourceLanguage(shaderc_source_language_glsl);
		}
		else if (m_Language == ShaderLanguage::HLSL)
		{
			options.SetSourceLanguage(shaderc_source_language_hlsl);
			options.SetAutoSampledTextures(true);
		}

		const std::unordered_map<String, String>& globalMacroses = Renderer::GetGlobalShaderMacroses();
		for (const auto& [name, value] : globalMacroses)
		{
			options.AddMacroDefinition(name, value);
		}

		for (const auto& [stage, cachePath, source] : result.StageDescriptions)
		{
			String filePathStr = m_FilePath.string();

			shaderc::PreprocessedSourceCompilationResult preResult =
				compiler.PreprocessGlsl(source, Utils::ShaderStageToShaderCKind(stage), filePathStr.c_str(), options);

			if (preResult.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				ATN_CORE_ERROR_TAG("Renderer", "Shader '{}'({}) preprocess failed, error message:\n{}\n", m_Name, Utils::ShaderStageToString(stage), preResult.GetErrorMessage());
				compiled = false;
				break;
			}

			String preProcessedSource = String(preResult.begin(), preResult.end());

			shaderc::SpvCompilationResult module =
				compiler.CompileGlslToSpv(preProcessedSource, Utils::ShaderStageToShaderCKind(stage), filePathStr.c_str(), GetEntryPoint(stage).data(), options);

			if (module.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				ATN_CORE_ERROR_TAG("Renderer", "Shader '{}'({}) compilation failed, error message:\n{}\n", m_Name, Utils::ShaderStageToString(stage), module.GetErrorMessage());
				compiled = false;
				break;
			}

			m_SPIRVBinaries[stage] = std::vector<uint32>(module.cbegin(), module.cend());

			// Save to cache
			auto& data = m_SPIRVBinaries.at(stage);
			FileSystem::WriteFile(cachePath, (const char*)data.data(), data.size() * sizeof(uint32));
		}

		return compiled;
	}

	void ShaderCompiler::ReadFromCache(const PreProcessResult& result)
	{
		for (const auto& [stage, cachePath, source] : result.StageDescriptions)
		{
			std::vector<byte> bytes = FileSystem::ReadFileBinary(cachePath);

			m_SPIRVBinaries[stage] = std::vector<uint32>(bytes.size() / sizeof(uint32));
			memcpy(m_SPIRVBinaries[stage].data(), bytes.data(), bytes.size());
		}
	}

	ShaderCompiler::PreProcessResult ShaderCompiler::PreProcess()
	{
		PreProcessResult result;
		result.ParseResult = true;

		if (!FileSystem::Exists(m_FilePath))
		{
			ATN_CORE_ERROR_TAG("Vulkan", "Invalid filepath for shader {}!", m_FilePath);
			result.ParseResult = false;
			return result;
		}

		result.StageDescriptions = PreProcessShaderStages();
		result.ParseResult = CheckShaderStages(result.StageDescriptions);

		result.NeedRecompile = false;
		for (const auto& [_, path, __] : result.StageDescriptions)
		{
			if (!FileSystem::Exists(path))
			{
				result.NeedRecompile = true;
				break;
			}
		}

		return result;
	}

	std::vector<ShaderCompiler::StageDescription> ShaderCompiler::PreProcessShaderStages()
	{
		if (m_Language == ShaderLanguage::GLSL)
			return GetGLSLStageDescriptions();

		if (m_Language == ShaderLanguage::HLSL)
			return GetHLSLStageDescriptions();

		return {};
	}

	std::vector<ShaderCompiler::StageDescription> ShaderCompiler::GetHLSLStageDescriptions()
	{
		std::vector<StageDescription> result;

		const char* matrixMemoryLayoutSettings = "#pragma pack_matrix( row_major )\n";

		String source = FileSystem::ReadFile(m_FilePath);
		source.insert(0, matrixMemoryLayoutSettings);

		for (const auto& [stage, entryPoint] : m_StageToEntryPointMap)
		{
			if (source.find(entryPoint) != std::string::npos)
			{
				StageDescription stageDesc;
				stageDesc.Stage = stage;
				stageDesc.FilePathToCache = Utils::GetCachedFilePath(m_FilePath, stage);
				stageDesc.Source = source;

				result.push_back(stageDesc);
			}
		}

		return result;
	}

	std::vector<ShaderCompiler::StageDescription> ShaderCompiler::GetGLSLStageDescriptions()
	{
		std::vector<StageDescription> result;

		String source = FileSystem::ReadFile(m_FilePath);

		const std::string_view pragmaToken = "#pragma stage : ";
		const std::string_view versionToken = "#version";

		uint64 pos = source.find(versionToken, 0);
		while (pos != std::string::npos)
		{
			uint64 stagePos = source.find(pragmaToken, pos + versionToken.size());
			uint64 eol = source.find_first_of("\r\n", stagePos);
			String stageString = source.substr(stagePos + pragmaToken.size(), eol - stagePos - pragmaToken.size());
			ShaderStage stage = Utils::GetShaderStageFromGLSLString(stageString);

			if (stage == ShaderStage::UNDEFINED)
			{
				ATN_CORE_ERROR_TAG("Renderer", "Failed to parse glsl shader stage!");
				return {};
			}

			uint64 nextPos = source.find(versionToken, pos + versionToken.size());
			String stageSource = source.substr(pos, nextPos - pos);

			// Add empty lines, for debugger
			uint64 linesCount = std::count(source.begin(), source.begin() + pos, '\n');
			String linesStr(linesCount, '\n');

			if(linesCount)
				stageSource.insert(0, linesStr);

			pos = nextPos;
			if (nextPos == std::string::npos)
				nextPos = source.size() - 1;

			StageDescription stageDesc;
			stageDesc.Stage = stage;
			stageDesc.FilePathToCache = Utils::GetCachedFilePath(m_FilePath, stage);
			stageDesc.Source = stageSource;

			result.push_back(stageDesc);
		}

		return result;
	}

	bool ShaderCompiler::CheckShaderStages(const std::vector<StageDescription>& stages)
	{
		std::string_view errorMsg = "";

		if (stages.size() == 0)
		{
			errorMsg = "Found 0 shader stages.";
		}

		std::unordered_map<ShaderStage, bool> shaderStageExistMap;
		shaderStageExistMap[ShaderStage::VERTEX_STAGE] = false;
		shaderStageExistMap[ShaderStage::FRAGMENT_STAGE] = false;
		shaderStageExistMap[ShaderStage::GEOMETRY_STAGE] = false;
		shaderStageExistMap[ShaderStage::COMPUTE_STAGE] = false;

		for (const auto& stage : stages)
		{
			shaderStageExistMap.at(stage.Stage) = true;
		}

		//if (shaderStageExistMap.at(ShaderStage::VERTEX_STAGE) && !shaderStageExistMap.at(ShaderStage::FRAGMENT_STAGE))
		//{
		//	errorMsg = "Has VERTEX_STAGE but no FRAGMENT_STAGE.";
		//}

		//if (!shaderStageExistMap.at(ShaderStage::VERTEX_STAGE) && shaderStageExistMap.at(ShaderStage::FRAGMENT_STAGE))
		//{
		//	errorMsg = "Has FRAGMENT_STAGE but no VERTEX_STAGE.";
		//}

		//if (shaderStageExistMap.at(ShaderStage::GEOMETRY_STAGE) && (!shaderStageExistMap.at(ShaderStage::VERTEX_STAGE) || shaderStageExistMap.at(ShaderStage::FRAGMENT_STAGE)))
		//{
		//	errorMsg = "Has GEOMETRY_STAGE but no VERTEX_SHADER or FRAGMENT_STAGE.";
		//}

		//if (shaderStageExistMap.at(ShaderStage::COMPUTE_STAGE) && stages.size() != 1)
		//{
		//	errorMsg = "Compute shader must have only 1 stage - COMPUTE_STAGE.";
		//}

		if (errorMsg.size() != 0)
		{
			ATN_CORE_ERROR_TAG("Renderer", "Shader '{}' compilation failed, error message:\n{}\n", m_Name, errorMsg);
			return false;
		}

		return true;
	}

	void ShaderCompiler::GetLanguageAndEntryPoints()
	{
		String extension = m_FilePath.extension().string();

		if (extension == ".hlsl")
		{
			m_Language = ShaderLanguage::HLSL;

			m_StageToEntryPointMap[ShaderStage::VERTEX_STAGE] = "VSMain";
			m_StageToEntryPointMap[ShaderStage::FRAGMENT_STAGE] = "FSMain";
			m_StageToEntryPointMap[ShaderStage::GEOMETRY_STAGE] = "GSMain";
			m_StageToEntryPointMap[ShaderStage::COMPUTE_STAGE] = "CSMain";
		}
		else if (extension == ".glsl")
		{
			m_Language = ShaderLanguage::GLSL;

			m_StageToEntryPointMap[ShaderStage::VERTEX_STAGE] = "main";
			m_StageToEntryPointMap[ShaderStage::FRAGMENT_STAGE] = "main";
			m_StageToEntryPointMap[ShaderStage::GEOMETRY_STAGE] = "main";
			m_StageToEntryPointMap[ShaderStage::COMPUTE_STAGE] = "main";
		}
		else
		{
			m_Language = ShaderLanguage::NONE;
		}
	}

	ShaderMetaData ShaderCompiler::Reflect()
	{
		ShaderMetaData result;
		result.PushConstant.Size = 0;
		result.WorkGroupSize = { 0, 0, 0 };
		result.PushConstant.StageFlags = ShaderStage::UNDEFINED;

		ATN_CORE_INFO_TAG("Renderer", "Reflecting Shader '{}'", m_Name);

		for (const auto& [stage, src] : m_SPIRVBinaries)
		{
			spirv_cross::Compiler compiler(src);
			spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			// WORK GROUP SIZE
			if (stage == ShaderStage::COMPUTE_STAGE)
			{
				auto entryPoints = compiler.get_entry_points_and_stages();
				for (const auto& entry : entryPoints)
				{
					if (entry.execution_model == spv::ExecutionModelGLCompute || entry.execution_model == spv::ExecutionModelKernel)
					{
						const spirv_cross::SPIREntryPoint& spirEntry = compiler.get_entry_point(entry.name, entry.execution_model);
						result.WorkGroupSize.x = spirEntry.workgroup_size.x;
						result.WorkGroupSize.y = spirEntry.workgroup_size.y;
						result.WorkGroupSize.z = spirEntry.workgroup_size.z;
					}
				}
			}

			// PUSH CONSTANTS
			if (result.PushConstant.Size != 0)
			{
				result.PushConstant.StageFlags = ShaderStage(result.PushConstant.StageFlags | stage);
			}
			else if(resources.push_constant_buffers.size() > 0)
			{
				spirv_cross::Resource resource = resources.push_constant_buffers[0];

				const auto& bufferType = compiler.get_type(resource.base_type_id);
				uint32 bufferSize = compiler.get_declared_struct_size(bufferType);
				int memberCount = bufferType.member_types.size();

				ATN_CORE_ASSERT(bufferSize <= 128, "Push constant block is bigger than 128 bytes!");

				result.PushConstant.Size = bufferSize;
				result.PushConstant.Members.reserve(memberCount);
				result.PushConstant.StageFlags = stage;

				uint32 memberOffset = 0;

				for (int i = 0; i < memberCount; ++i)
				{
					String name = compiler.get_member_name(resource.base_type_id, i);
					size_t size = compiler.get_declared_struct_member_size(compiler.get_type(resource.base_type_id), i);

					const auto& spirvType = compiler.get_type(compiler.get_type(resource.base_type_id).member_types[i]);
					ShaderDataType elemType = Utils::SpirvTypeToShaderDataType(spirvType);

					StructMemberShaderMetaData memberData;
					memberData.Size = size;
					memberData.Offset = memberOffset;
					memberData.Type = elemType;

					result.PushConstant.Members[name] = memberData;

					memberOffset += size;
				}
			}

			// SAMPLED IMAGES
			for (const auto& resource : resources.sampled_images)
			{
				if (result.SampledTextures.contains(resource.name))
				{
					auto& stageFlags = result.SampledTextures.at(resource.name).StageFlags;
					stageFlags = ShaderStage(stageFlags | stage);
				}
				else
				{
					const auto& dim = compiler.get_type(resource.base_type_id).image.dim;
					uint32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
					uint32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
					const auto& array = compiler.get_type(resource.type_id).array;

					TextureShaderMetaData textureData;
					textureData.ImageType = Utils::SpirvDimToImageType(dim);
					textureData.Binding = binding;
					textureData.Set = set;
					textureData.StageFlags = stage;
					textureData.ArraySize = array.empty() ? 1 : array[0];

					result.SampledTextures[resource.name] = textureData;
				}
			}

			// STORAGE IMAGES
			for (const auto& resource : resources.storage_images)
			{
				if (result.StorageTextures.contains(resource.name))
				{
					auto& stageFlags = result.StorageTextures.at(resource.name).StageFlags;
					stageFlags = ShaderStage(stageFlags | stage);
				}
				else
				{
					const auto& dim = compiler.get_type(resource.base_type_id).image.dim;
					uint32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
					uint32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
					const auto& array = compiler.get_type(resource.type_id).array;

					TextureShaderMetaData textureData;
					textureData.ImageType = Utils::SpirvDimToImageType(dim);
					textureData.Binding = binding;
					textureData.Set = set;
					textureData.StageFlags = stage;
					textureData.ArraySize = array.empty() ? 1 : array[0];

					result.StorageTextures[resource.name] = textureData;
				}
			}

			// SEPARATE SAMPLERS
			for (const auto& resource : resources.separate_samplers)
			{
				if (result.Samplers.contains(resource.name))
				{
					auto& stageFlags = result.Samplers.at(resource.name).StageFlags;
					stageFlags = ShaderStage(stageFlags | stage);
				}
				else
				{
					uint32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
					uint32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
					const auto& array = compiler.get_type(resource.type_id).array;

					SamplerShaderMetaData samplerData;
					samplerData.Binding = binding;
					samplerData.Set = set;
					samplerData.StageFlags = stage;
					samplerData.ArraySize = array.empty() ? 1 : array[0];

					result.Samplers[resource.name] = samplerData;
				}
			}

			// UNIFORM BUFFERS
			for (const auto& resource : resources.uniform_buffers)
			{
				if (result.UniformBuffers.contains(resource.name))
				{
					auto& stageFlags = result.UniformBuffers.at(resource.name).StageFlags;
					stageFlags = ShaderStage(stageFlags | stage);
				}
				else
				{
					const auto& bufferType = compiler.get_type(resource.base_type_id);
					uint32 bufferSize = compiler.get_declared_struct_size(bufferType);
					uint32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
					uint32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
					const auto& array = compiler.get_type(resource.type_id).array;

					BufferShaderMetaData bufferData;
					bufferData.Size = bufferSize;
					bufferData.Binding = binding;
					bufferData.Set = set;
					bufferData.StageFlags = stage;
					bufferData.ArraySize = array.empty() ? 1 : array[0];

					result.UniformBuffers[resource.name] = bufferData;
				}
			}

			// STORAGE BUFFERS
			for (const auto& resource : resources.storage_buffers)
			{
				// Spirv-cross seems to be treating storage buffers differently (resource.name is incorrect)
				// So we make work around and get resource name using base_type_id
				String resourceName = compiler.get_name(resource.base_type_id);

				if (result.StorageBuffers.contains(resourceName))
				{
					auto& stageFlags = result.StorageBuffers.at(resourceName).StageFlags;
					stageFlags = ShaderStage(stageFlags | stage);
				}
				else
				{
					const auto& bufferType = compiler.get_type(resource.base_type_id);
					uint32 bufferSize = compiler.get_declared_struct_size(bufferType);
					uint32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
					uint32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
					const auto& array = compiler.get_type(resource.type_id).array;

					BufferShaderMetaData bufferData;
					bufferData.Size = bufferSize;
					bufferData.Binding = binding;
					bufferData.Set = set;
					bufferData.StageFlags = stage;
					bufferData.ArraySize = array.empty() ? 1 : array[0];

					result.StorageBuffers[resourceName] = bufferData;
				}
			}
		}

		result.PushConstant.Enabled = result.PushConstant.Size != 0;

		ATN_CORE_TRACE("push constant: {} members, {} bytes", result.PushConstant.Members.size(), result.PushConstant.Size);
		for (const auto& [name, member] : result.PushConstant.Members)
			ATN_CORE_TRACE("\t{}: {} bytes, {} offset", name, member.Size, member.Offset);

		ATN_CORE_TRACE("sampled textures: {}", result.SampledTextures.size());
		for (const auto& [name, texture] : result.SampledTextures)
			ATN_CORE_TRACE("\t{}: binding {}, set {}, arraySize {}", name, texture.Binding, texture.Set, texture.ArraySize);

		ATN_CORE_TRACE("storage textures: {}", result.StorageTextures.size());
		for (const auto& [name, texture] : result.StorageTextures)
			ATN_CORE_TRACE("\t{}: binding {}, set {}, arraySize {}", name, texture.Binding, texture.Set, texture.ArraySize);

		ATN_CORE_TRACE("uniform buffers: {}", result.UniformBuffers.size());
		for (const auto& [name, buffer] : result.UniformBuffers)
			ATN_CORE_TRACE("\t{}: {} bytes, binding {}, set {}, arraySize {}", name, buffer.Size, buffer.Binding, buffer.Set, buffer.ArraySize);

		ATN_CORE_TRACE("storage buffers : {}", result.StorageBuffers.size());
		for (const auto& [name, buffer] : result.StorageBuffers)
			ATN_CORE_TRACE("\t{}: {} bytes, binding {}, set {}, arraySize {}", name, buffer.Size, buffer.Binding, buffer.Set, buffer.ArraySize);

		return result;
	}
}
