#include "Shader.h"
#include "Athena/Core/Application.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/Time.h"
#include "Athena/Renderer/ShaderCompiler.h"
#include "Athena/Renderer/RHI/RHI.h"
#include "Athena/Core/Yaml.h"

#include <xxhash/xxhash.h>


namespace Athena
{
	void ShaderManager::Init()
	{
		m_ShaderSourcesPath = Application::Get().GetRendererConfig().ShaderSourcesPath;
		m_ShadersCachePath = Application::Get().GetConfig().EngineCachePath / "CompiledShaders";

		for (const auto& def : m_Definitions)
		{
			def->Init();
		}
	}

	void ShaderManager::Destroy()
	{
		for (const auto& def : m_Definitions)
		{
			def->Destroy();
		}

		m_Definitions.clear();
	}

	void ShaderManager::CompileAll()
	{
		Ref<ShaderCompiler> compiler = ShaderCompiler::Create();
		ShaderModel shaderModel = Application::Get().GetRendererConfig().ShaderModel;

		for (const auto& def : m_Definitions)
		{
			String source = GetShaderSource(def);
			if (source.empty())
			{
				ATN_LOG_ERROR(Renderer, "Failed to read shader file contents from {}", def->GetRelativeFilePath());
				continue;
			}

			uint32 permutationIndex = 0;

			for (PermutationIterator it(def->GetDomain()); it; ++it, permutationIndex++)
			{
				Timer compilationTimer;

				String shaderName = fmt::format("{}_Permutation{}", def->GetRelativeFilePath().stem().string(), permutationIndex);

				const PermutationVector& vector = it.GetVector();

				// 1. create environment
				const ShaderCompilerEnvironment& env = compiler->SetupEnvironment(shaderModel, def, vector, shaderName);

				if (!def->GetShader()->ShouldCompilePermutation(vector, env))
					continue;

				// 2. get hash
				ShaderPreprocessResult preprocessResult = compiler->Preprocess(source);

				if (!preprocessResult.Success)
					continue;

				uint64 hash = GetShaderHash(preprocessResult.PreprocessedSource, env);
				ShaderCacheLocation cacheLoc = GetShaderCacheLocation(def->GetRelativeFilePath(), env.Target, hash);

				// 3. check if we need to compile
				bool validCache = IsCacheValid(cacheLoc);

				// 4. compile or get from cache
				Ref<ShaderInstance> instance;
				if (validCache)
				{
					instance = GetShaderFromCache(cacheLoc);
				}
				else
				{
					ShaderCompilationResult compilationResult = compiler->Compile(preprocessResult.PreprocessedSource);

					if (compilationResult.Success)
					{
						SaveCache(cacheLoc, compilationResult.Bytecode, compilationResult.ReflectionData);
						instance = ShaderInstance::Create(compilationResult.Bytecode, compilationResult.ReflectionData);
					}
				}

				if (instance)
				{
					def->AddInstance(instance, permutationIndex);

					ATN_LOG_INFO(Renderer, "Shader {} stage = {} {} took {}", shaderName, EnumUtils::ShaderTypeToString(env.ShaderStage),
						validCache ? "reading from cache" : "compilation", compilationTimer.ElapsedTime());
				}
			}
		}
	}

	String ShaderManager::GetShaderSource(const ShaderDefinition* def) const
	{
		FilePath path = m_ShaderSourcesPath / def->GetRelativeFilePath();
		String sourceCode = FileSystem::ReadFile(path);

		return sourceCode;
	}

	uint64 ShaderManager::GetShaderHash(const String& preprocessedSource, const ShaderCompilerEnvironment& env) const
	{
		uint64 hash = XXH3_64bits(preprocessedSource.data(), preprocessedSource.size());
		HashCombine(hash, env.GetHash());

		return hash;
	}

	ShaderManager::ShaderCacheLocation ShaderManager::GetShaderCacheLocation(const FilePath& relativePath, ShaderTarget target, uint64 hash) const
	{
		FilePath directory = m_ShadersCachePath / EnumUtils::ShaderTargetToString(target) / relativePath.parent_path() / relativePath.stem();
		FilePath filename = fmt::format("{}_{}.{}", relativePath.stem(), hash, GetShaderTargetFileExtension(target));

		ShaderCacheLocation result;
		result.ByteCodePath = directory / filename;
		result.ReflectionDataPath = result.ByteCodePath;
		result.ReflectionDataPath.replace_extension("reflection");

		return result;
	}

	bool ShaderManager::IsCacheValid(const ShaderCacheLocation& cacheLoc) const
	{
		return FileSystem::Exists(cacheLoc.ByteCodePath) && FileSystem::Exists(cacheLoc.ReflectionDataPath);
	}

	Ref<ShaderInstance> ShaderManager::GetShaderFromCache(const ShaderCacheLocation& cacheLoc) const
	{
		Buffer bytecode = FileSystem::ReadFileBinary(cacheLoc.ByteCodePath);

		ShaderReflectionData reflectionData;
		reflectionData.Deserialize(cacheLoc.ReflectionDataPath);

		return ShaderInstance::Create(bytecode, reflectionData);
	}

	void ShaderManager::SaveCache(const ShaderCacheLocation& cacheLoc, const Buffer& bytecode, const ShaderReflectionData& data) const
	{
		FileSystem::WriteFile(cacheLoc.ByteCodePath, (const char*)bytecode.Data(), bytecode.Size());
		data.Serialize(cacheLoc.ReflectionDataPath);
	}

	void ShaderManager::RegisterDefinition(ShaderDefinition* def)
	{
		m_Definitions.push_back(def);
	}

	ShaderDefinition::ShaderDefinition(Shader* shader, const FilePath& filepath, const String& entryPoint, ShaderType type)
		: m_Shader(shader)
		, m_FilePath(filepath)
		, m_EntryPoint(entryPoint)
		, m_Type(type)
	{
		ShaderManager::Get().RegisterDefinition(this);
	}

	void ShaderDefinition::Init()
	{
		m_Domain = m_Shader->BuildPermutationDomain();
	}

	void ShaderDefinition::Destroy()
	{
		delete m_Shader;
		m_Instances.clear();
	}

	void ShaderDefinition::AddInstance(const Ref<ShaderInstance>& instance, uint32 permutationIndex)
	{
		ensuref(!m_Instances.contains(permutationIndex));
		m_Instances[permutationIndex] = instance;
	}

	Ref<ShaderInstance> ShaderDefinition::GetInstance(const PermutationVector& vector) const
	{
		uint32 permutationIndex = PermutationIterator::GetPermutationIndex(GetDomain(), vector);
		if (!m_Instances.contains(permutationIndex))
		{
			ensuref(false);
			if (m_Instances.empty())
				return nullptr;
			
			return m_Instances.begin()->second;
		}

		return m_Instances.at(permutationIndex);
	}


	Ref<ShaderInstance> ShaderInstance::Create(const Buffer& bytecode, const ShaderReflectionData& reflectionData)
	{
		return RHI::Get()->CreateShaderInstance(bytecode, reflectionData);
	}

	ShaderInstance::ShaderInstance(const ShaderReflectionData& reflectionData)
		: m_ReflectionData(reflectionData)
	{

	}

	void ShaderReflectionData::Serialize(const FilePath& path) const
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "ShaderReflectionData" << YAML::Value << YAML::BeginMap;
	
		out << YAML::Key << "InputsCount" << YAML::Value << InputsCount;
		out << YAML::Key << "OutputsCount" << YAML::Value << OutputsCount;
		out << YAML::Key << "WorkGroupSize" << YAML::Value << WorkGroupSize;

		out << YAML::Key << "Textures" << YAML::BeginMap;
		for (const auto& [name, texture] : Textures)
		{
			out << YAML::Key << name << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "TextureType" << YAML::Value << EnumUtils::TextureTypeToString(texture.TextureType);
			out << YAML::Key << "BindPoint" << YAML::Value << texture.Binding.BindPoint;
			out << YAML::Key << "Set" << YAML::Value << texture.Binding.Set;
			out << YAML::Key << "ArraySize" << YAML::Value << texture.Binding.ArraySize;
			out << YAML::Key << "ReadOnly" << YAML::Value << texture.ReadOnly;
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		out << YAML::Key << "Samplers" << YAML::BeginMap;
		for (const auto& [name, sampler] : Samplers)
		{
			out << YAML::Key << name << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "BindPoint" << YAML::Value << sampler.Binding.BindPoint;
			out << YAML::Key << "Set" << YAML::Value << sampler.Binding.Set;
			out << YAML::Key << "ArraySize" << YAML::Value << sampler.Binding.ArraySize;
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		out << YAML::Key << "StructuredBuffers" << YAML::BeginMap;
		for (const auto& [name, buffer] : StructuredBuffers)
		{
			out << YAML::Key << name << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "BindPoint" << YAML::Value << buffer.Binding.BindPoint;
			out << YAML::Key << "Set" << YAML::Value << buffer.Binding.Set;
			out << YAML::Key << "ArraySize" << YAML::Value << buffer.Binding.ArraySize;
			out << YAML::Key << "ReadOnly" << YAML::Value << buffer.ReadOnly;
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		out << YAML::Key << "ByteAddressBuffers" << YAML::BeginMap;
		for (const auto& [name, buffer] : ByteAddressBuffers)
		{
			out << YAML::Key << name << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "BindPoint" << YAML::Value << buffer.Binding.BindPoint;
			out << YAML::Key << "Set" << YAML::Value << buffer.Binding.Set;
			out << YAML::Key << "ArraySize" << YAML::Value << buffer.Binding.ArraySize;
			out << YAML::Key << "ReadOnly" << YAML::Value << buffer.ReadOnly;
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		out << YAML::Key << "UniformBuffers" << YAML::BeginMap;
		for (const auto& [name, buffer] : UniformBuffers)
		{
			out << YAML::Key << name << YAML::Value << YAML::BeginMap;
			out << YAML::Key << "Size" << YAML::Value << buffer.Size;
			out << YAML::Key << "BindPoint" << YAML::Value << buffer.Binding.BindPoint;
			out << YAML::Key << "Set" << YAML::Value << buffer.Binding.Set;
			out << YAML::Key << "ArraySize" << YAML::Value << buffer.Binding.ArraySize;
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		out << YAML::EndMap << YAML::EndMap;

		std::ofstream fout(path);
		fout << out.c_str();
	}

	void ShaderReflectionData::Deserialize(const FilePath& path)
	{
		YAML::Node data = YAML::TryLoadYAMLFile(path);
		ensuref(data);

		const YAML::Node reflection = data["ShaderReflectionData"];

		InputsCount = reflection["InputsCount"].as<uint32>();
		OutputsCount = reflection["OutputsCount"].as<uint32>();
		WorkGroupSize = reflection["WorkGroupSize"].as<Vector3u>();

		const YAML::Node textures = reflection["Textures"];
		for (const auto& texture : textures)
		{
			const String name = texture.first.as<String>();
			const YAML::Node value = texture.second;

			TextureReflectionData data;
			data.TextureType = EnumUtils::TextureTypeFromString(value["TextureType"].as<String>());

			data.Binding.BindPoint = value["BindPoint"].as<uint32_t>();
			data.Binding.Set = value["Set"].as<uint32_t>();
			data.Binding.ArraySize = value["ArraySize"].as<uint32_t>();
			data.ReadOnly = value["ReadOnly"].as<bool>();

			Textures.emplace(name, std::move(data));
		}

		const YAML::Node samplers = reflection["Samplers"];
		for (const auto& sampler : samplers)
		{
			const String name = sampler.first.as<String>();
			const YAML::Node value = sampler.second;

			SamplerReflectionData data;
			data.Binding.BindPoint = value["BindPoint"].as<uint32_t>();
			data.Binding.Set = value["Set"].as<uint32_t>();
			data.Binding.ArraySize = value["ArraySize"].as<uint32_t>();

			Samplers.emplace(name, std::move(data));
		}

		const YAML::Node structuredBuffers = reflection["StructuredBuffers"];
		for (const auto& buffer : structuredBuffers)
		{
			const String name = buffer.first.as<String>();
			const YAML::Node value = buffer.second;

			BufferReflectionData data;
			data.Binding.BindPoint = value["BindPoint"].as<uint32_t>();
			data.Binding.Set = value["Set"].as<uint32_t>();
			data.Binding.ArraySize = value["ArraySize"].as<uint32_t>();
			data.ReadOnly = value["ReadOnly"].as<bool>();

			StructuredBuffers.emplace(name, std::move(data));
		}

		const YAML::Node byteAddressBuffers = reflection["ByteAddressBuffers"];
		for (const auto& buffer : byteAddressBuffers)
		{
			const String name = buffer.first.as<String>();
			const YAML::Node value = buffer.second;

			BufferReflectionData data;
			data.Binding.BindPoint = value["BindPoint"].as<uint32_t>();
			data.Binding.Set = value["Set"].as<uint32_t>();
			data.Binding.ArraySize = value["ArraySize"].as<uint32_t>();
			data.ReadOnly = value["ReadOnly"].as<bool>();

			ByteAddressBuffers.emplace(name, std::move(data));
		}

		const YAML::Node uniformBuffers = reflection["UniformBuffers"];
		for (const auto& buffer : uniformBuffers)
		{
			const String name = buffer.first.as<String>();
			const YAML::Node value = buffer.second;

			UniformBufferReflectionData data;
			data.Size = value["Size"].as<uint32_t>();
			data.Binding.BindPoint = value["BindPoint"].as<uint32_t>();
			data.Binding.Set = value["Set"].as<uint32_t>();
			data.Binding.ArraySize = value["ArraySize"].as<uint32_t>();

			UniformBuffers.emplace(name, std::move(data));
		}
	}

}
