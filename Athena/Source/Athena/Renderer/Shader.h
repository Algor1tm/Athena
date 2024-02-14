#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"

#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Image.h"


namespace Athena
{
	enum ShaderStage
	{
		UNDEFINED      = BIT(0),
		VERTEX_STAGE   = BIT(1),
		FRAGMENT_STAGE = BIT(2),
		GEOMETRY_STAGE = BIT(3),
		COMPUTE_STAGE  = BIT(4)
	};

	struct StructMemberShaderMetaData
	{
		ShaderDataType Type;	// can be Unknown
		uint32 Size;
		uint32 Offset;
	};

	struct PushConstantShaderMetaData
	{
		bool Enabled;
		uint32 Size;
		std::unordered_map<String, StructMemberShaderMetaData> Members;
		ShaderStage StageFlags;
	};

	struct BufferShaderMetaData
	{
		uint64 Size;
		uint32 Binding;
		uint32 Set;
		uint32 ArraySize;
		ShaderStage StageFlags;
	};

	struct TextureShaderMetaData
	{
		ImageType ImageType;
		uint32 Binding;
		uint32 Set;
		uint32 ArraySize;
		ShaderStage StageFlags;
	};

	struct ShaderMetaData
	{
		VertexBufferLayout VertexBufferLayout;

		std::unordered_map<String, TextureShaderMetaData> SampledTextures;
		std::unordered_map<String, TextureShaderMetaData> StorageTextures;
		std::unordered_map<String, BufferShaderMetaData> UniformBuffers;
		std::unordered_map<String, BufferShaderMetaData> StorageBuffers;
		PushConstantShaderMetaData PushConstant;
	};

	class ATHENA_API Shader : public RefCounted
	{
	public:
		static Ref<Shader> Create(const FilePath& path);
		static Ref<Shader> Create(const FilePath& path, const String& name);

		virtual ~Shader() = default;

		virtual void Reload() = 0;
		virtual bool IsCompute() = 0;

		const ShaderMetaData& GetMetaData() { return m_MetaData; };
		bool IsCompiled() const { return m_IsCompiled; }
		const String& GetName() const { return m_Name; }

	protected:
		String m_Name;
		FilePath m_FilePath;
		bool m_IsCompiled;
		ShaderMetaData m_MetaData;
	};

	class ATHENA_API ShaderPack : public RefCounted
	{
	public:
		static Ref<ShaderPack> Create(const FilePath& path);
		void Add(const String& name, const Ref<Shader>& shader);

		Ref<Shader> Load(const FilePath& path);
		Ref<Shader> Load(const FilePath& path, const String& name);
		Ref<Shader> Get(const String& name);

		bool Exists(const String& name);
		void Reload();

		const FilePath GetDirectory() const { return m_Directory; };

	private:
		void LoadDirectory(const FilePath& path);

	private:
		std::unordered_map<String, Ref<Shader>> m_Shaders;
		FilePath m_Directory;
	};
}
