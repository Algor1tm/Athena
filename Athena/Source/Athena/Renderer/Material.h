#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/RenderCommandBuffer.h"


namespace Athena
{
	class ATHENA_API Material : public RefCounted
	{
	public:
		static Ref<Material> Create(const Ref<Shader>& shader, const String& name);
		static Ref<Material> CreatePBRStatic(const String& name);
		static Ref<Material> CreatePBRAnim(const String& name);
		virtual ~Material();

		void Set(const String& name, const Matrix4& value);
		void Set(const String& name, const Vector4& value);
		void Set(const String& name, float value);
		void Set(const String& name, uint32 value);

		virtual void Set(const String& name, const Ref<RenderResource>& resource, uint32 arrayIndex = 0) = 0;
		virtual void Set(const String& name, const Ref<Texture>& resource, uint32 arrayIndex, uint32 mip) = 0;

		template <typename T>
		T Get(const String& name);

		virtual void Bind(const Ref<RenderCommandBuffer>& commandBuffer) = 0;
		void RT_UpdateForRendering(const Ref<RenderCommandBuffer>& commandBuffer);

		Ref<Shader> GetShader() const { return m_Shader; }
		const String& GetName() const { return m_Name; }
		uint64 GetHash() const { return m_Hash; }

	protected:
		Material(const Ref<Shader> shader, const String& name);

	private:
		virtual void RT_SetPushConstant(const Ref<RenderCommandBuffer>& commandBuffer, const void* data) = 0;
		virtual Ref<RenderResource> GetResourceInternal(const String& name) = 0;
		virtual void OnReload() = 0;

		bool TryGetMemberData(const String& name, ShaderDataType dataType, StructMemberShaderMetaData* memberData);
		void SetInternal(const String& name, ShaderDataType dataType, const void* data);
		bool GetInternal(const String& name, ShaderDataType dataType, void** data);

	private:
		Ref<Shader> m_Shader;
		String m_Name;
		byte m_Buffer[128];
		const std::unordered_map<String, StructMemberShaderMetaData>* m_BufferMembers;
		uint64 m_Hash;
	};

	template <>
	inline Matrix4 Material::Get<Matrix4>(const String& name)
	{
		void* data;
		if (GetInternal(name, ShaderDataType::Mat4, &data))
			return *(Matrix4*)data;

		return Matrix4(0);
	}

	template <>
	inline Vector4 Material::Get<Vector4>(const String& name)
	{
		void* data;
		if (GetInternal(name, ShaderDataType::Float4, &data))
			return *(Vector4*)data;

		return Vector4(0);
	}

	template <>
	inline float Material::Get<float>(const String& name)
	{
		void* data;
		if (GetInternal(name, ShaderDataType::Float, &data))
			return *(float*)data;

		return float(0);
	}

	template <>
	inline uint32 Material::Get<uint32>(const String& name)
	{
		void* data;
		if (GetInternal(name, ShaderDataType::UInt, &data))
			return *(uint32*)data;

		return uint32(0);
	}

	template <>
	inline Ref<Texture2D> Material::Get<Ref<Texture2D>>(const String& name)
	{
		return GetResourceInternal(name);
	}


	class ATHENA_API MaterialTable : public RefCounted
	{
	public:
		Ref<Material> Get(const String& name) const;
		void Add(const Ref<Material>& material);
		void Remove(const Ref<Material>& material);

		bool Exists(const String& name) const;

	private:
		std::unordered_map<String, Ref<Material>> m_Materials;
	};
}
