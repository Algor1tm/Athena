#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	class ATHENA_API Texture2D;
	class ATHENA_API Material;

	enum class MaterialTexture
	{
		ALBEDO_MAP,
		NORMAL_MAP,
		ROUGHNESS_MAP,
		METALNESS_MAP,
		AMBIENT_OCCLUSION_MAP
	};

	enum class MaterialUniform
	{
		ALBEDO,
		ROUGHNESS,
		METALNESS,
		EMISSION
	};

	class ATHENA_API MaterialManager
	{
	public:
		static Ref<Material> CreateMaterial(const String& name = "UnNamed");

		static bool Exists(const String& name);
		static Ref<Material> Get(const String& name);
		static void Delete(const String& name);

		static auto GetMaterialsMapIterator() { return m_Materials.cbegin(); };
		static uint32 GetMaterialsCount() { return m_Materials.size(); };

	private:
		static std::unordered_map<String, Ref<Material>> m_Materials;
	};


	class ATHENA_API Material
	{
	public:
		friend class ATHENA_API MaterialManager;

	public:
		struct ShaderData
		{
			LinearColor Albedo = LinearColor::White;
			float Roughness = 0.7f;
			float Metalness = 0.f;
			float Emission = 0.f;

			int EnableAlbedoMap;
			int EnableNormalMap;
			int EnableRoughnessMap;
			int EnableMetalnessMap;
			int EnableAmbientOcclusionMap;
		};

	public:
		template <typename T>
		inline void Set(MaterialUniform uniform, T value);
		void Set(MaterialTexture textureType, const Ref<Texture2D>& texture);

		template <typename T>
		inline T Get(MaterialUniform uniform) const;
		Ref<Texture2D> Get(MaterialTexture textureType);

		bool IsEnabled(MaterialTexture textureType) const;
		void Enable(MaterialTexture textureType, bool enable);

		const ShaderData& Bind();
		const String& GetName() const { return m_Name; };

		bool operator==(const Material& other) const
		{
			return m_Name == other.m_Name;
		}

		bool operator!=(const Material& other) const
		{
			return m_Name != other.m_Name;
		}

	private:
		void BindTexture(MaterialTexture textureType, TextureBinder binder, int* isEnabled);

		struct TextureInfo
		{
			Ref<Texture2D> Texture;
			bool IsEnabled = true;
		};

	private:
		ShaderData m_ShaderData;
		std::unordered_map<MaterialTexture, TextureInfo> m_TextureMap;

		String m_Name;
	};


	template <>
	inline void Material::Set<float>(MaterialUniform uniform, float value)
	{
		switch (uniform)
		{
		case MaterialUniform::ROUGHNESS: m_ShaderData.Roughness = value; break;
		case MaterialUniform::METALNESS: m_ShaderData.Metalness = value; break;
		case MaterialUniform::EMISSION: m_ShaderData.Emission = value; break;
		default: ATN_CORE_ERROR_TAG("Material", "Invalid uniform in Material::Set<float>");
		}
	}

	template <>
	inline void Material::Set<Vector3>(MaterialUniform uniform, Vector3 value)
	{
		switch (uniform)
		{
		case MaterialUniform::ALBEDO: m_ShaderData.Albedo = value; break;
		default: ATN_CORE_ERROR_TAG("Material", "Invalid uniform in Material::Set<Vector3>");
		}
	}

	template <>
	inline float Material::Get<float>(MaterialUniform uniform) const
	{
		switch (uniform)
		{
		case MaterialUniform::ROUGHNESS: return m_ShaderData.Roughness;
		case MaterialUniform::METALNESS: return m_ShaderData.Metalness;
		case MaterialUniform::EMISSION:  return m_ShaderData.Emission;
		default: ATN_CORE_ERROR_TAG("Material", "Invalid uniform in Material::Get<float>");
		}

		return 0.f;
	}

	template <>
	inline Vector3 Material::Get<Vector3>(MaterialUniform uniform) const
	{
		switch (uniform)
		{
		case MaterialUniform::ALBEDO: return m_ShaderData.Albedo;
		default: ATN_CORE_ERROR_TAG("Material", "Invalid uniform in Material::Get<Vector3>");
		}

		return Vector3(0.f);
	}
}
