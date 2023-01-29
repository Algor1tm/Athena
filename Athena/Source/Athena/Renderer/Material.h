#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/ConstantBuffer.h"


namespace Athena
{
	struct MaterialDescription
	{
		Vector3 Albedo = Vector3(1);
		float Roughness = 0;
		float Metalness = 0;
		float AmbientOcclusion = 1;

		Ref<Texture2D> AlbedoTexture = nullptr;
		Ref<Texture2D> NormalMap = nullptr;
		Ref<Texture2D> RoughnessMap = nullptr;
		Ref<Texture2D> MetalnessMap = nullptr;
		Ref<Texture2D> AmbientOcclusionMap = nullptr;

		bool UseAlbedoTexture = false;
		bool UseNormalMap = false;
		bool UseRoughnessMap = false;
		bool UseMetalnessMap = false;
		bool UseAmbientOcclusionMap = false;
	};

	class ATHENA_API Material;

	class ATHENA_API MaterialManager
	{
	public:
		static Ref<Material> CreateMaterial(const MaterialDescription& desc, const String& name = "UnNamed");

		static Ref<Material> GetMaterial(const String& name);
		static void DeleteMaterial(const String& name);

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
			LinearColor Albedo;
			float Roughness;
			float Metalness;
			float AmbientOcclusion;

			int UseAlbedoTexture;
			int UseNormalMap;
			int UseRoughnessMap;
			int UseMetalnessMap;
			int UseAmbientOcclusionMap;
		};

	public:
		const ShaderData& Bind();
		MaterialDescription& GetDescription() { return m_Description; };
		const String& GetName() const { return m_Name; };

		bool operator==(const Material& other)
		{
			return m_Name == other.m_Name;
		}

	private:
		ShaderData m_ShaderData;
		MaterialDescription m_Description;
		String m_Name;
	};
}
