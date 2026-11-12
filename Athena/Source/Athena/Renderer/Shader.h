#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Buffer.h"
#include "Athena/Renderer/ShaderCompilerEnvironment.h"
#include "Athena/Renderer/ShaderPermutations.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	struct ResourceBindingData
	{
		uint32 BindPoint;
		uint32 Set;
		uint32 ArraySize;
	};

	struct TextureReflectionData
	{
		TextureType TextureType;
		ResourceBindingData Binding;
		bool ReadOnly;
	};

	struct SamplerReflectionData
	{
		ResourceBindingData Binding;
	};

	struct BufferReflectionData
	{
		ResourceBindingData Binding;
		bool ReadOnly;
	};

	struct UniformBufferReflectionData
	{
		uint64 Size;
		ResourceBindingData Binding;
	};

	struct ShaderReflectionData
	{
		std::unordered_map<String, TextureReflectionData> Textures;
		std::unordered_map<String, SamplerReflectionData> Samplers;
		std::unordered_map<String, BufferReflectionData> StructuredBuffers;
		std::unordered_map<String, BufferReflectionData> ByteAddressBuffers;
		std::unordered_map<String, UniformBufferReflectionData> UniformBuffers;

		uint32 InputsCount;
		uint32 OutputsCount;
		Vector3u WorkGroupSize;

		void Serialize(const FilePath& path) const;
		void Deserialize(const FilePath& path);
	};

	class ATHENA_API ShaderInstance : public RefCounted
	{
	public:
		static Ref<ShaderInstance> Create(const Buffer& bytecode, const ShaderReflectionData& reflectionData);

		ShaderInstance(const ShaderReflectionData& reflectionData);
		virtual ~ShaderInstance() = default;

		const ShaderReflectionData& GetReflectionData() const { return m_ReflectionData; }

	private:
		ShaderReflectionData m_ReflectionData;
	};

	class ATHENA_API Shader
	{
	public:
		Shader() = default;
		virtual ~Shader() = default;
		NON_COPYABLE_NON_MOVABLE(Shader);

		virtual PermutationDomain BuildPermutationDomain() { return {}; };
		virtual void ModifyCompilerEnvironment(const PermutationVector& permutation, ShaderCompilerEnvironment& env) {};
		virtual bool ShouldCompilePermutation(const PermutationVector& permutation, const ShaderCompilerEnvironment& env) { return true; }
	};

	class ShaderDefinition;

	class ATHENA_API ShaderManager
	{
	public:
		ShaderManager() = default;
		NON_COPYABLE_NON_MOVABLE(ShaderManager);

		static ShaderManager& Get()
		{
			static ShaderManager s_Instance;
			return s_Instance;
		}

		void Init();
		void Destroy();

		void CompileAll();

		void RegisterDefinition(ShaderDefinition* def);
		FilePath GetShaderSourcesPath() { return m_ShaderSourcesPath; }

	private:
		struct ShaderCacheLocation
		{
			FilePath ByteCodePath;
			FilePath ReflectionDataPath;
		};

		String GetShaderSource(const ShaderDefinition* def) const;
		uint64 GetShaderHash(const String& preprocessedSource, const ShaderCompilerEnvironment& env) const;

		ShaderCacheLocation GetShaderCacheLocation(const FilePath& relativePath, ShaderTarget target, uint64 hash) const;

		bool IsCacheValid(const ShaderCacheLocation& cacheLoc) const;
		Ref<ShaderInstance> GetShaderFromCache(const ShaderCacheLocation& cacheLoc) const;
		void SaveCache(const ShaderCacheLocation& cacheLoc, const Buffer& bytecode, const ShaderReflectionData& data) const;

	private:
		std::vector<ShaderDefinition*> m_Definitions;
		FilePath m_ShaderSourcesPath;
		FilePath m_ShadersCachePath;
	};


	class ATHENA_API ShaderDefinition
	{
	public:
		ShaderDefinition(Shader* shader, const FilePath& filepath, const String& entryPoint, ShaderType type);
		NON_COPYABLE_NON_MOVABLE(ShaderDefinition);

		void Init();
		void Destroy();

		Shader* GetShader() const { return m_Shader; }
		const PermutationDomain& GetDomain() const { return m_Domain; }
		const FilePath& GetRelativeFilePath() const { return m_FilePath; };
		const String& GetEntryPoint() const { return m_EntryPoint; }
		ShaderType GetType() const { return m_Type; }

		void AddInstance(const Ref<ShaderInstance>& instance, uint32 permutationIndex);
		Ref<ShaderInstance> GetInstance(const PermutationVector& vector) const;

	private:
		Shader* m_Shader;
		PermutationDomain m_Domain;
		FilePath m_FilePath;
		String m_EntryPoint;
		ShaderType m_Type;
		std::unordered_map<uint32, Ref<ShaderInstance>> m_Instances;
	};

#define IMPLEMENT_SHADER(varName, shaderClass, path, entry, type) ShaderDefinition varName(new shaderClass(), path, entry, type);
#define EXPORT_SHADER(varName) extern ShaderDefinition varName;

}
