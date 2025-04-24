#include "Material.h"

#include "Athena/Renderer/MaterialAsset.h"
#include "Athena/Renderer/TextureGenerator.h"


namespace Athena
{
	Ref<MaterialAsset> MaterialAsset::Create()
	{
		Ref<MaterialAsset> result = Ref<MaterialAsset>::Create();
		result->m_Material = Material::CreatePBR();

		result->SetAlbedo(result->GetAlbedo());
		result->SetEmission(result->GetEmission());
		result->SetRoughness(result->GetRoughness());
		result->SetMetalness(result->GetMetalness());

		result->m_TexturesMap[MaterialTextureType::Albedo] = AssetHandle(0);
		result->m_TexturesMap[MaterialTextureType::Normals] = AssetHandle(0);
		result->m_TexturesMap[MaterialTextureType::Roughness] = AssetHandle(0);
		result->m_TexturesMap[MaterialTextureType::Metalness] = AssetHandle(0);

		result->m_TexturesFlags[MaterialTextureType::Albedo] = false;
		result->m_TexturesFlags[MaterialTextureType::Normals] = false;
		result->m_TexturesFlags[MaterialTextureType::Roughness] = false;
		result->m_TexturesFlags[MaterialTextureType::Metalness] = false;

		return result;
	}

	Ref<MaterialAsset> MaterialAsset::GetDefault()
	{
		static Ref<MaterialAsset> s_DefaultMaterial;

		if (s_DefaultMaterial == nullptr)
		{
			s_DefaultMaterial = MaterialAsset::Create();
		}

		return s_DefaultMaterial;
	}

	const LinearColor& MaterialAsset::GetAlbedo() const
	{
		return m_Albedo;
	}

	void MaterialAsset::SetAlbedo(const LinearColor& albedo)
	{
		m_Albedo = Math::Clamp(Vector4(albedo), 0.f, 1.f);
		m_Material->Set("u_Albedo", Vector4(m_Albedo));
	}

	float MaterialAsset::GetEmission() const
	{
		return m_Emission;
	}

	void MaterialAsset::SetEmission(float emission)
	{
		m_Emission = emission;
		if (m_Emission < 0)
			m_Emission = 0;

		m_Material->Set("u_Emission", m_Emission);
	}

	float MaterialAsset::GetRoughness() const
	{
		return m_Roughness;
	}

	void MaterialAsset::SetRoughness(float roughness)
	{
		m_Roughness = Math::Clamp(roughness, 0.f, 1.f);
		m_Material->Set("u_Roughness", m_Roughness);
	}

	float MaterialAsset::GetMetalness() const
	{
		return m_Metalness;
	}

	void MaterialAsset::SetMetalness(float metalness)
	{
		m_Metalness = Math::Clamp(metalness, 0.f, 1.f);
		m_Material->Set("u_Metalness", m_Metalness);
	}

	AssetHandle MaterialAsset::GetTexture(MaterialTextureType type) const
	{
		return m_TexturesMap.at(type);
	}

	void MaterialAsset::SetTexture(MaterialTextureType type, AssetHandle texture)
	{
		m_TexturesMap.at(type) = texture;
	}

	void MaterialAsset::RemoveTexture(MaterialTextureType type)
	{
		m_TexturesMap.at(type) = 0;
		EnableTexture(type, false);
	}

	bool MaterialAsset::IsEnabledTexture(MaterialTextureType type) const
	{
		return m_TexturesFlags.at(type);
	}

	void MaterialAsset::EnableTexture(MaterialTextureType type, bool flag)
	{
		m_TexturesFlags.at(type) = flag;

		switch (type)
		{
		case MaterialTextureType::Albedo:     m_Material->Set("u_UseAlbedoMap", (uint32)flag); break;
		case MaterialTextureType::Normals:    m_Material->Set("u_UseNormalMap", (uint32)flag); break;
		case MaterialTextureType::Roughness:  m_Material->Set("u_UseRoughnessMap", (uint32)flag); break;
		case MaterialTextureType::Metalness:  m_Material->Set("u_UseMetalnessMap", (uint32)flag); break;
		}
	}

	bool MaterialAsset::IsFlagSet(MaterialFlag flag) const
	{
		return m_Material->IsFlagSet(flag);
	}

	void MaterialAsset::SetFlag(MaterialFlag flag, bool value)
	{
		m_Material->SetFlag(flag, value);
	}

	void MaterialAsset::UpdateTextureAssets()
	{
		Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(m_TexturesMap.at(MaterialTextureType::Albedo));
		m_Material->Set("u_AlbedoMap", texture ? texture : TextureGenerator::GetWhiteTexture());

		texture = AssetManager::GetAsset<Texture2D>(m_TexturesMap.at(MaterialTextureType::Normals));
		m_Material->Set("u_NormalMap", texture ? texture : TextureGenerator::GetWhiteTexture());

		texture = AssetManager::GetAsset<Texture2D>(m_TexturesMap.at(MaterialTextureType::Roughness));
		m_Material->Set("u_RoughnessMap", texture ? texture : TextureGenerator::GetWhiteTexture());

		texture = AssetManager::GetAsset<Texture2D>(m_TexturesMap.at(MaterialTextureType::Metalness));
		m_Material->Set("u_MetalnessMap", texture ? texture : TextureGenerator::GetWhiteTexture());
	}
}
