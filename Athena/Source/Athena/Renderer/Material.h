#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class ATHENA_API Material : public RefCounted
	{
	public:
		static Ref<Material> Create(const Ref<Shader>& shader, const String& name);
		static Ref<Material> CreatePBRStatic(const String& name);
		virtual ~Material() = default;

		virtual void Set(const String& name, const Ref<ShaderResource>& resource) = 0;

		void Set(const String& name, const Matrix4& value);
		void Set(const String& name, const Vector4& value);
		void Set(const String& name, float value);
		void Set(const String& name, uint32 value);

		template <typename T>
		T Get(const String& name);

		virtual void Bind() = 0;
		virtual void RT_UpdateForRendering() = 0;

		Ref<Shader> GetShader() { return m_Shader; }
		const String& GetName() { return m_Name; }

	private:
		virtual void SetInternal(const String& name, ShaderDataType dataType, const void* data) = 0;
		virtual bool GetInternal(const String& name, ShaderDataType dataType, void** data) = 0;

		virtual Ref<Texture2D> GetTexture(const String& name) = 0;

	protected:
		Ref<Shader> m_Shader;
		String m_Name;
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
		return GetTexture(name);
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
