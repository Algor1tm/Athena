#include "Material.h"

#include "Athena/Renderer/MaterialAsset.h"
#include "Athena/Renderer/EngineTextures.h"

#include "Athena/Core/YAMLTypes.h"


namespace Athena
{
	MaterialAsset::MaterialAsset()
	{
		m_Material = Material::CreatePBR();

		SetAlbedo(GetAlbedo());
		SetEmission(GetEmission());
		SetRoughness(GetRoughness());
		SetMetalness(GetMetalness());

		m_TexturesMap[MaterialTextureType::Albedo] = AssetHandle(0);
		m_TexturesMap[MaterialTextureType::Normals] = AssetHandle(0);
		m_TexturesMap[MaterialTextureType::Roughness] = AssetHandle(0);
		m_TexturesMap[MaterialTextureType::Metalness] = AssetHandle(0);

		m_TexturesFlags[MaterialTextureType::Albedo] = false;
		m_TexturesFlags[MaterialTextureType::Normals] = false;
		m_TexturesFlags[MaterialTextureType::Roughness] = false;
		m_TexturesFlags[MaterialTextureType::Metalness] = false;
	}

	Ref<MaterialAsset> MaterialAsset::GetDefault()
	{
		static Ref<MaterialAsset> s_DefaultMaterial;

		if (s_DefaultMaterial == nullptr)
		{
			s_DefaultMaterial = Ref<MaterialAsset>::Create();
		}

		return s_DefaultMaterial;
	}

	bool MaterialAsset::Serialize(const FilePath& absolutePath) const
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;

		out << YAML::Key << "Albedo" << YAML::Value << GetAlbedo();
		out << YAML::Key << "Emission" << YAML::Value << GetEmission();
		out << YAML::Key << "Roughness" << YAML::Value << GetRoughness();
		out << YAML::Key << "Metalness" << YAML::Value << GetMetalness();

		out << YAML::Key << "AlbedoMap" << YAML::Value << GetTexture(MaterialTextureType::Albedo);
		out << YAML::Key << "NormalMap" << YAML::Value << GetTexture(MaterialTextureType::Normals);
		out << YAML::Key << "RoughnessMap" << YAML::Value << GetTexture(MaterialTextureType::Roughness);
		out << YAML::Key << "MetalnessMap" << YAML::Value << GetTexture(MaterialTextureType::Metalness);

		out << YAML::Key << "UseAlbedoMap" << YAML::Value << IsEnabledTexture(MaterialTextureType::Albedo);
		out << YAML::Key << "UseNormalMap" << YAML::Value << IsEnabledTexture(MaterialTextureType::Normals);
		out << YAML::Key << "UseRoughnessMap" << YAML::Value << IsEnabledTexture(MaterialTextureType::Roughness);
		out << YAML::Key << "UseMetalnessMap" << YAML::Value << IsEnabledTexture(MaterialTextureType::Metalness);

		out << YAML::Key << "CastShadows" << YAML::Value << IsFlagSet(MaterialFlag::CastShadows);

		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream fout(absolutePath);
		fout << out.c_str();

		return true;
	}

	bool MaterialAsset::Deserialize(const FilePath& absolutePath, Ref<AssetImportSettings> settings)
	{
		YAML::Node data = YAML::TryLoadYAMLFile(absolutePath);
		if (!data)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to load material asset data from {}!", absolutePath);
			return false;
		}

		YAML::Node root = data["Material"];
		if (!root)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to load material asset data from {}!", absolutePath);
			return false;
		}

		SetAlbedo(TryReadYAMLValue<LinearColor>(root, "Albedo", LinearColor::Black));
		SetEmission(TryReadYAMLValue<float>(root, "Emission", 0.f));
		SetRoughness(TryReadYAMLValue<float>(root, "Roughness", 0.f));
		SetMetalness(TryReadYAMLValue<float>(root, "Metalness", 0.f));

		SetTexture(MaterialTextureType::Albedo, TryReadYAMLValue<AssetHandle>(root, "AlbedoMap", 0));
		SetTexture(MaterialTextureType::Normals, TryReadYAMLValue<AssetHandle>(root, "NormalMap", 0));
		SetTexture(MaterialTextureType::Roughness, TryReadYAMLValue<AssetHandle>(root, "RoughnessMap", 0));
		SetTexture(MaterialTextureType::Metalness, TryReadYAMLValue<AssetHandle>(root, "MetalnessMap", 0));

		EnableTexture(MaterialTextureType::Albedo, TryReadYAMLValue<bool>(root, "UseAlbedoMap", false));
		EnableTexture(MaterialTextureType::Normals, TryReadYAMLValue<bool>(root, "UseNormalMap", false));
		EnableTexture(MaterialTextureType::Roughness, TryReadYAMLValue<bool>(root, "UseRoughnessMap", false));
		EnableTexture(MaterialTextureType::Metalness, TryReadYAMLValue<bool>(root, "UseMetalnessMap", false));

		SetFlag(MaterialFlag::CastShadows, TryReadYAMLValue<bool>(root, "CastShadows", true));

		return true;
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

	void MaterialAsset::SetTexture(MaterialTextureType type, AssetHandle textureHandle)
	{
		m_TexturesMap.at(type) = textureHandle;

		Ref<TextureAsset> texture = AssetManager::GetAsset<TextureAsset>(textureHandle);
		m_Material->Set(TextureTypeToShaderString(type), texture ? texture->GetRenderTexture() : EngineTextures::GetWhiteTexture());
	}

	void MaterialAsset::RemoveTexture(MaterialTextureType type)
	{
		m_TexturesMap.at(type) = 0;
		EnableTexture(type, false);

		m_Material->Set(TextureTypeToShaderString(type), EngineTextures::GetWhiteTexture());
	}

	bool MaterialAsset::IsEnabledTexture(MaterialTextureType type) const
	{
		return m_TexturesFlags.at(type);
	}

	void MaterialAsset::EnableTexture(MaterialTextureType type, bool flag)
	{
		m_TexturesFlags.at(type) = flag;
		m_Material->Set(TextureTypeEnableToShaderString(type), (uint32)flag);
	}

	bool MaterialAsset::IsFlagSet(MaterialFlag flag) const
	{
		return m_Material->IsFlagSet(flag);
	}

	void MaterialAsset::SetFlag(MaterialFlag flag, bool value)
	{
		m_Material->SetFlag(flag, value);
	}

	String MaterialAsset::TextureTypeToShaderString(MaterialTextureType type) const
	{
		switch (type)
		{
		case MaterialTextureType::Albedo:     return "u_AlbedoMap";
		case MaterialTextureType::Normals:    return "u_NormalMap";
		case MaterialTextureType::Roughness:  return "u_RoughnessMap";
		case MaterialTextureType::Metalness:  return "u_MetalnessMap";
		}

		ATN_CORE_ASSERT(false);
		return "";
	}

	String MaterialAsset::TextureTypeEnableToShaderString(MaterialTextureType type) const
	{
		switch (type)
		{
		case MaterialTextureType::Albedo:     return "u_UseAlbedoMap";
		case MaterialTextureType::Normals:    return "u_UseNormalMap";
		case MaterialTextureType::Roughness:  return "u_UseRoughnessMap";
		case MaterialTextureType::Metalness:  return "u_UseMetalnessMap";
		}

		ATN_CORE_ASSERT(false);
		return "";
	}
}
