#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/RenderCommandBuffer.h"


namespace Athena
{
	class ATHENA_API PushConstantRange
	{
	public:
		PushConstantRange(const Ref<Shader>& shader);

		void Set(const String& name, const Matrix4& value);
		void Set(const String& name, const Vector4& value);
		void Set(const String& name, float value);
		void Set(const String& name, uint32 value);

		template <typename T>
		T Get(const String& name);

		const void* GetData() const { return m_Buffer; }

	private:
		bool TryGetMemberData(const String& name, ShaderDataType dataType, StructMemberShaderMetaData* memberData);

		void SetInternal(const String& name, ShaderDataType dataType, const void* data);
		bool GetInternal(const String& name, ShaderDataType dataType, void** data);

	private:
		const std::unordered_map<String, StructMemberShaderMetaData>* m_Members;
		byte m_Buffer[128];
	};

	template <>
	inline Matrix4 PushConstantRange::Get<Matrix4>(const String& name)
	{
		void* data;
		if (GetInternal(name, ShaderDataType::Mat4, &data))
			return *(Matrix4*)data;

		return Matrix4(0);
	}

	template <>
	inline Vector4 PushConstantRange::Get<Vector4>(const String& name)
	{
		void* data;
		if (GetInternal(name, ShaderDataType::Float4, &data))
			return *(Vector4*)data;

		return Vector4(0);
	}

	template <>
	inline float PushConstantRange::Get<float>(const String& name)
	{
		void* data;
		if (GetInternal(name, ShaderDataType::Float, &data))
			return *(float*)data;

		return float(0);
	}

	template <>
	inline uint32 PushConstantRange::Get<uint32>(const String& name)
	{
		void* data;
		if (GetInternal(name, ShaderDataType::UInt, &data))
			return *(uint32*)data;

		return uint32(0);
	}


	class ATHENA_API Material : public RefCounted
	{
	public:
		static Ref<Material> Create(const Ref<Shader>& shader, const String& name);
		static Ref<Material> CreatePBRStatic(const String& name);
		static Ref<Material> CreatePBRAnim(const String& name);
		virtual ~Material() = default;

		template <typename T>
		void Set(const String& name, const T& value)
		{
			m_PushConstantRange.Set(name, value);
		}

		template <typename T>
		T Get(const String& name)
		{
			return m_PushConstantRange.Get<T>(name);
		}

		virtual void Bind(const Ref<RenderCommandBuffer>& commandBuffer) = 0;
		void RT_UpdateForRendering(const Ref<RenderCommandBuffer>& commandBuffer);

		Ref<Shader> GetShader() { return m_Shader; }
		const String& GetName() { return m_Name; }

	protected:
		Material(const Ref<Shader> shader, const String& name);

	private:
		virtual void SetResource(const String& name, const Ref<ShaderResource>& resource) = 0;
		virtual Ref<ShaderResource> GetResource(const String& name) = 0;

		virtual void RT_SetPushConstant(const Ref<RenderCommandBuffer>& commandBuffer, const PushConstantRange& range) = 0;

	private:
		Ref<Shader> m_Shader;
		String m_Name;
		PushConstantRange m_PushConstantRange;
	};

	template <>
	inline void Material::Set<Ref<Texture2D>>(const String& name, const Ref<Texture2D>& resource)
	{
		SetResource(name, resource);
	}

	template <>
	inline Ref<Texture2D> Material::Get<Ref<Texture2D>>(const String& name)
	{
		return GetResource(name);
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
