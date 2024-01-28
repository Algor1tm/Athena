#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"

#include "Athena/Renderer/GPUBuffers.h"


namespace Athena
{
	enum ShaderStage
	{
		VERTEX_STAGE   = BIT(1),
		FRAGMENT_STAGE = BIT(2),
		GEOMETRY_STAGE = BIT(3),
		COMPUTE_STAGE  = BIT(4)
	};

	struct StructMemberReflectionData
	{
		ShaderDataType Type;	// can be Unknown
		uint32 Size;
		uint32 Offset;
	};

	struct PushConstantReflectionData
	{
		bool Enabled;
		uint32 Size;
		std::unordered_map<String, StructMemberReflectionData> Members;
		ShaderStage StageFlags;
	};

	struct BufferReflectionData
	{
		uint64 Size;
		uint32 Binding;
		uint32 Set;
		ShaderStage StageFlags;
	};

	struct SampledTextureReflectionData
	{
		uint32 Binding;
		uint32 Set;
		ShaderStage StageFlags;
	};

	struct ShaderReflectionData
	{
		VertexBufferLayout VertexBufferLayout;

		std::unordered_map<String, BufferReflectionData> UniformBuffers;
		std::unordered_map<String, SampledTextureReflectionData> SampledTextures;
		PushConstantReflectionData PushConstant;
	};

	class ATHENA_API Shader : public RefCounted
	{
	public:
		static Ref<Shader> Create(const FilePath& path);
		static Ref<Shader> Create(const FilePath& path, const String& name);

		virtual ~Shader() = default;

		const ShaderReflectionData& GetReflectionData() { return m_ReflectionData; };

		bool IsCompiled() const { return m_IsCompiled; }
		virtual void Reload() = 0;

		const String& GetName() const { return m_Name; }

	protected:
		String m_Name;
		FilePath m_FilePath;
		bool m_IsCompiled;
		ShaderReflectionData m_ReflectionData;
	};

	class ATHENA_API ShaderPack : public RefCounted
	{
	public:
		void Add(const String& name, const Ref<Shader>& shader);

		Ref<Shader> Load(const String& name, const FilePath& path);
		Ref<Shader> Get(const String& name);

		bool Exists(const String& name);
		void Reload();

	private:
		std::unordered_map<String, Ref<Shader>> m_Shaders;
	};
}
