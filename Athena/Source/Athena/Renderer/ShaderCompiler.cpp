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

		static ShaderDataType SprivTypeToShaderDataType(const spirv_cross::SPIRType& type)
		{
			// Scalar
			if (type.vecsize == 1 && type.columns == 1)
			{
				switch (type.basetype)
				{
				case spirv_cross::SPIRType::BaseType::Int: return ShaderDataType::Int;
				case spirv_cross::SPIRType::BaseType::Float: return ShaderDataType::Float;
				case spirv_cross::SPIRType::BaseType::Boolean: return ShaderDataType::Bool;
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
		options.SetHlslOffsets(true);
		options.SetInvertY(true);
		options.SetAutoSampledTextures(true);
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

		const char* matrixMemoryLayoutSettings = "#pragma pack_matrix( row_major )\n\n";

		result.Source = FileSystem::ReadFile(m_FilePath);
		result.Source.insert(0, matrixMemoryLayoutSettings);

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
			ATN_CORE_ERROR_TAG("Renderer", "Shader '{}' compilation failed, error message:\n{}\n", m_Name, errorMsg);
			return false;
		}

		return true;
	}

	ShaderReflectionData ShaderCompiler::Reflect()
	{
		ShaderReflectionData result;
		result.PushConstant.Size = 0;

		ATN_CORE_INFO_TAG("Renderer", "Reflecting Shader '{}'", m_Name);

		for (const auto& [stage, src] : m_SPIRVBinaries)
		{
			spirv_cross::Compiler compiler(src);
			spirv_cross::ShaderResources resources = compiler.get_shader_resources();

			if (stage == ShaderStage::VERTEX_STAGE)
			{
				std::vector<VertexBufferElement> elements;
				elements.reserve(resources.stage_inputs.size());
				for (const auto& resource : resources.stage_inputs)
				{
					ShaderDataType elemType = Utils::SprivTypeToShaderDataType(compiler.get_type(resource.type_id));
					ATN_CORE_ASSERT(elemType != ShaderDataType::Unknown);

					elements.push_back({ elemType, resource.name });
				}

				result.VertexBufferLayout = elements;
			}

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
					ShaderDataType elemType = Utils::SprivTypeToShaderDataType(spirvType);

					StructMemberReflectionData memberData;
					memberData.Size = size;
					memberData.Offset = memberOffset;
					memberData.Type = elemType;

					result.PushConstant.Members[name] = memberData;

					memberOffset += size;
				}
			}

			for (const auto& resource : resources.uniform_buffers)
			{
				const auto& bufferType = compiler.get_type(resource.base_type_id);
				uint32 bufferSize = compiler.get_declared_struct_size(bufferType);
				uint32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				uint32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

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

			for (const auto& resource : resources.sampled_images)
			{
				uint32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
				uint32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

				if (result.Textures2D.contains(resource.name))
				{
					auto& stageFlags = result.Textures2D.at(resource.name).StageFlags;
					stageFlags = ShaderStage(stageFlags | stage);
				}
				else
				{
					Texture2DReflectionData textureData;
					textureData.Binding = binding;
					textureData.Set = set;
					textureData.StageFlags = stage;

					result.Textures2D[resource.name] = textureData;
				}
			}
		}

		result.PushConstant.Enabled = result.PushConstant.Size != 0;

		ATN_CORE_TRACE("  vertex inputs: {}", result.VertexBufferLayout.GetElementsNum());
		for (const auto& elem : result.VertexBufferLayout)
			ATN_CORE_TRACE("\t input {}: {} bytes", elem.Name, elem.Size);

		ATN_CORE_TRACE("  push constant: {} members, {} bytes", result.PushConstant.Members.size(), result.PushConstant.Size);
		for (const auto& [name, member] : result.PushConstant.Members)
			ATN_CORE_TRACE("\t {}: {} bytes, {} offset", name, member.Size, member.Offset);

		ATN_CORE_TRACE("  uniform buffers: {}", result.UniformBuffers.size());
		for (const auto& [name, buffer] : result.UniformBuffers)
			ATN_CORE_TRACE("\t buffer {}: {} bytes, binding {}, set {}", name, buffer.Size, buffer.Binding, buffer.Set);

		ATN_CORE_TRACE("  sampled textures: {}", result.Textures2D.size());
		for (const auto& [name, texture] : result.Textures2D)
			ATN_CORE_TRACE("\t texture {}: binding {}, set {}", name, texture.Binding, texture.Set);

		return result;
	}
}
