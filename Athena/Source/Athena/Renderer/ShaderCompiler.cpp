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

		static ShaderDataType SprivTypeToShaderDataType(spirv_cross::SPIRType type)
		{
			// Scalar
			if (type.vecsize == 1 && type.columns == 1)
			{
				switch (type.basetype)
				{
				case spirv_cross::SPIRType::BaseType::Int: return ShaderDataType::Int;
				case spirv_cross::SPIRType::BaseType::Float: return ShaderDataType::Float;
				case spirv_cross::SPIRType::BaseType::Boolean: return ShaderDataType::Bool;
				default: return ShaderDataType::None;
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
					default: return ShaderDataType::None;
					}
				}
				case spirv_cross::SPIRType::BaseType::Float:
				{
					switch (type.vecsize)
					{
					case 2: return ShaderDataType::Float2;
					case 3: return ShaderDataType::Float3;
					case 4: return ShaderDataType::Float4;
					default: return ShaderDataType::None;
					}
				}
				default: return ShaderDataType::None;
				}
			}

			// Matrix
			if (type.vecsize != 1 && type.columns != 1 && type.vecsize == type.columns)
			{
				switch (type.vecsize)
				{
				case 3: return ShaderDataType::Mat3;
				case 4: return ShaderDataType::Mat4;
				default: return ShaderDataType::None;
				}
			}

			return ShaderDataType::None;
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

		m_StageToEntryPointMap[ShaderStage::VERTEX_STAGE] = "VSMain";
		m_StageToEntryPointMap[ShaderStage::FRAGMENT_STAGE] = "FSMain";
		m_StageToEntryPointMap[ShaderStage::GEOMETRY_STAGE] = "GSMain";
		m_StageToEntryPointMap[ShaderStage::COMPUTE_STAGE] = "CSMain";
	}

	std::string_view ShaderCompiler::GetEntryPoint(ShaderStage stage) const
	{
		ATN_CORE_ASSERT(m_StageToEntryPointMap.contains(stage));
		return m_StageToEntryPointMap.at(stage);
	}

	bool ShaderCompiler::CompileOrGetFromCache(bool forceCompile)
	{
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
			ATN_CORE_INFO_TAG("Vulkan", "Shader '{}' compilation took {}", m_Name, compilationTimer.ElapsedTime());

		return compiled;
	}

	bool ShaderCompiler::CompileAndWriteToCache(const PreProcessResult& result)
	{
		bool compiled = true;

		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		options.SetSourceLanguage(shaderc_source_language_hlsl);
		options.SetInvertY(true);
		options.SetGenerateDebugInfo();
		options.SetIncluder(std::make_unique<FileSystemIncluder>());

		const std::unordered_map<String, String>& globalMacroses = Renderer::GetGlobalShaderMacroses();
		for (const auto& [name, value] : globalMacroses)
		{
			options.AddMacroDefinition(name, value);
		}

		for (const auto& [stage, cachePath] : result.StageDescriptions)
		{
			String filePathStr = m_FilePath.string();

			shaderc::PreprocessedSourceCompilationResult preResult =
				compiler.PreprocessGlsl(result.Source, Utils::ShaderStageToShaderCKind(stage), filePathStr.c_str(), options);

			if (preResult.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				ATN_CORE_ERROR_TAG("Vulkan", "Shader '{}' preprocess failed, error message:\n{}\n", m_Name, preResult.GetErrorMessage());
				compiled = false;
				break;
			}

			String preProcessedSource = String(preResult.begin(), preResult.end());

			shaderc::SpvCompilationResult module =
				compiler.CompileGlslToSpv(preProcessedSource, Utils::ShaderStageToShaderCKind(stage), filePathStr.c_str(), GetEntryPoint(stage).data(), options);

			if (module.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				ATN_CORE_ERROR_TAG("Vulkan", "Shader '{}' compilation failed, error message:\n{}\n", m_Name, module.GetErrorMessage());
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
		for (const auto& [stage, cachePath] : result.StageDescriptions)
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
		}

		FilePath ext = m_FilePath.extension();
		if (ext != L".hlsl")
		{
			ATN_CORE_ERROR_TAG("Vulkan", "Invalid shader extension {}!", m_FilePath);
			result.ParseResult = false;
		}

		if (result.ParseResult == false)
			return result;

		const char* macroSettings = "#pragma pack_matrix( row_major )\n\n";

		result.Source = FileSystem::ReadFile(m_FilePath);
		result.Source.insert(0, macroSettings);

		for (const auto& [stage, entryPoint] : m_StageToEntryPointMap)
		{
			if (result.Source.find(entryPoint) != std::string::npos)
			{
				StageDescription stageDesc;
				stageDesc.Stage = stage;
				stageDesc.FilePathToCache = Utils::GetCachedFilePath(m_FilePath, stage);

				result.StageDescriptions.push_back(stageDesc);
			}
		}

		result.ParseResult = CheckShaderStages(result.StageDescriptions);

		result.NeedRecompile = false;
		for (const auto& [_, path] : result.StageDescriptions)
		{
			if (!FileSystem::Exists(path))
			{
				result.NeedRecompile = true;
				break;
			}
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

		if (shaderStageExistMap.at(ShaderStage::VERTEX_STAGE) && !shaderStageExistMap.at(ShaderStage::FRAGMENT_STAGE))
		{
			errorMsg = "Has VERTEX_STAGE but no FRAGMENT_STAGE.";
		}

		if (!shaderStageExistMap.at(ShaderStage::VERTEX_STAGE) && shaderStageExistMap.at(ShaderStage::FRAGMENT_STAGE))
		{
			errorMsg = "Has FRAGMENT_STAGE but no VERTEX_STAGE.";
		}

		if (shaderStageExistMap.at(ShaderStage::GEOMETRY_STAGE) && (!shaderStageExistMap.at(ShaderStage::VERTEX_STAGE) || shaderStageExistMap.at(ShaderStage::FRAGMENT_STAGE)))
		{
			errorMsg = "Has GEOMETRY_STAGE but no VERTEX_SHADER or FRAGMENT_STAGE.";
		}

		if (shaderStageExistMap.at(ShaderStage::COMPUTE_STAGE) && stages.size() != 1)
		{
			errorMsg = "Compute shader must have only 1 stage - COMPUTE_STAGE.";
		}

		if (errorMsg.size() != 0)
		{
			ATN_CORE_ERROR_TAG("Vulkan", "Shader '{}' compilation failed, error message:\n{}\n", m_Name, errorMsg);
			return false;
		}

		return true;
	}

	ShaderReflectionData ShaderCompiler::Reflect()
	{
		ShaderReflectionData result;

		for (const auto& [stage, src] : m_SPIRVBinaries)
		{
			spirv_cross::Compiler compiler(src);
			spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			ATN_CORE_TRACE("Shader Reflect - {} {}", Utils::ShaderStageToString(stage), m_Name);
			ATN_CORE_TRACE("    {} uniform buffers", resources.uniform_buffers.size());
			ATN_CORE_TRACE("    {} resources", resources.sampled_images.size());

			if (stage == ShaderStage::VERTEX_STAGE)
			{
				ATN_CORE_TRACE("Vertex buffer layout:");

				std::vector<VertexBufferElement> elements;

				for (const auto& resource : resources.stage_inputs)
				{
					ATN_CORE_TRACE("  {}", resource.name);

					auto elemType = Utils::SprivTypeToShaderDataType(compiler.get_type(resource.type_id));
					ATN_CORE_ASSERT(elemType != ShaderDataType::None);

					elements.push_back({ elemType, resource.name });
				}

				result.VertexBufferLayout = elements;
			}

			ATN_CORE_TRACE("Uniform buffers:");
			for (const auto& resource : resources.uniform_buffers)
			{
				const auto& bufferType = compiler.get_type(resource.base_type_id);
				uint32 bufferSize = compiler.get_declared_struct_size(bufferType);
				uint32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				uint32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

				int memberCount = bufferType.member_types.size();

				ATN_CORE_TRACE("  {}", resource.name);
				ATN_CORE_TRACE("\tBuffer size = {}", bufferSize);
				ATN_CORE_TRACE("\tBinding = {}", binding);
				ATN_CORE_TRACE("\tSet = {}", set);
				ATN_CORE_TRACE("\tMembers = {}", memberCount);

				for (int i = 0; i < memberCount; ++i)
				{
					String name = compiler.get_member_name(resource.base_type_id, i);
					size_t size = compiler.get_declared_struct_member_size(compiler.get_type(resource.base_type_id), i);
					ATN_CORE_TRACE("\t  Name = {}", name);
					ATN_CORE_TRACE("\t  Size = {}", size);
				}

				if (result.UniformBuffers.contains(resource.name))
				{
					auto& stageFlags = result.UniformBuffers.at(resource.name).StageFlags;
					stageFlags = ShaderStage(stageFlags | stage);
				}
				else
				{
					BufferReflectionData bufferData;
					bufferData.Size = bufferSize;
					bufferData.Binding = binding;
					bufferData.Set = set;
					bufferData.StageFlags = stage;

					result.UniformBuffers[resource.name] = bufferData;
				}
			}
		}

		return result;
	}
}
