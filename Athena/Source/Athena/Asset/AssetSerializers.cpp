#include "AssetSerializers.h"

#include "Athena/Renderer/MaterialAsset.h"
#include "Athena/Scene/SceneSerializer.h"

#include "Athena/Core/YAMLTypes.h"


namespace Athena
{
	void MaterialSerializer::Serialize(const Ref<Asset>& asset, const AssetMetadata& metadata)
	{
		Ref<MaterialAsset> material = asset.As<MaterialAsset>();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;

		out << YAML::Key << "Albedo" << YAML::Value << material->GetAlbedo();
		out << YAML::Key << "Emission" << YAML::Value << material->GetEmission();
		out << YAML::Key << "Roughness" << YAML::Value << material->GetRoughness();
		out << YAML::Key << "Metalness" << YAML::Value << material->GetMetalness();

		out << YAML::Key << "AlbedoMap" << YAML::Value << material->GetTexture(MaterialTextureType::Albedo);
		out << YAML::Key << "NormalMap" << YAML::Value << material->GetTexture(MaterialTextureType::Normals);
		out << YAML::Key << "RoughnessMap" << YAML::Value << material->GetTexture(MaterialTextureType::Roughness);
		out << YAML::Key << "MetalnessMap" << YAML::Value << material->GetTexture(MaterialTextureType::Metalness);

		out << YAML::Key << "UseAlbedoMap" << YAML::Value << material->IsEnabledTexture(MaterialTextureType::Albedo);
		out << YAML::Key << "UseNormalMap" << YAML::Value << material->IsEnabledTexture(MaterialTextureType::Normals);
		out << YAML::Key << "UseRoughnessMap" << YAML::Value << material->IsEnabledTexture(MaterialTextureType::Roughness);
		out << YAML::Key << "UseMetalnessMap" << YAML::Value << material->IsEnabledTexture(MaterialTextureType::Metalness);

		out << YAML::Key << "CastShadows" << YAML::Value << material->IsFlagSet(MaterialFlag::CastShadows);

		out << YAML::EndMap;
		out << YAML::EndMap;

		FilePath absolutePath = AssetManager::GetAssetAbsolutePath(metadata.FilePath);

		std::ofstream fout(absolutePath);
		fout << out.c_str();
	}

	bool MaterialSerializer::TryLoadData(const Ref<Asset>& asset, const AssetMetadata& metadata)
	{
		FilePath absolutePath = AssetManager::GetAssetAbsolutePath(metadata.FilePath);

		if (!FileSystem::Exists(absolutePath))
			return false;

		YAML::Node data;
		try
		{
			data = YAML::LoadFile(absolutePath.string());

			auto materialNode = data["Material"];
			if (!materialNode)
				return false;

			Ref<MaterialAsset> material = asset.As<MaterialAsset>();

			material->SetAlbedo(materialNode["Albedo"].as<LinearColor>());
			material->SetEmission(materialNode["Emission"].as<float>());
			material->SetRoughness(materialNode["Roughness"].as<float>());
			material->SetMetalness(materialNode["Metalness"].as<float>());

			material->SetTexture(MaterialTextureType::Albedo, materialNode["AlbedoMap"].as<AssetHandle>());
			material->SetTexture(MaterialTextureType::Normals, materialNode["NormalMap"].as<AssetHandle>());
			material->SetTexture(MaterialTextureType::Roughness, materialNode["RoughnessMap"].as<AssetHandle>());
			material->SetTexture(MaterialTextureType::Metalness, materialNode["MetalnessMap"].as<AssetHandle>());

			material->EnableTexture(MaterialTextureType::Albedo, materialNode["UseAlbedoMap"].as<bool>());
			material->EnableTexture(MaterialTextureType::Normals, materialNode["UseNormalMap"].as<bool>());
			material->EnableTexture(MaterialTextureType::Roughness, materialNode["UseRoughnessMap"].as<bool>());
			material->EnableTexture(MaterialTextureType::Metalness, materialNode["UseMetalnessMap"].as<bool>());

			material->SetFlag(MaterialFlag::CastShadows, materialNode["CastShadows"].as<bool>());
		}
		catch (YAML::Exception& e)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to load material asset data from {}. Error message:\n {}", absolutePath, e.what());
			return false;
		}

		return true;
	}

	void SceneAssetSerializer::Serialize(const Ref<Asset>& asset, const AssetMetadata& metadata)
	{
		FilePath absolutePath = AssetManager::GetAssetAbsolutePath(metadata.FilePath);

		SceneSerializer serializer(asset);
		serializer.SerializeToFile(absolutePath);
	}

	bool SceneAssetSerializer::TryLoadData(const Ref<Asset>& asset, const AssetMetadata& metadata)
	{
		FilePath absolutePath = AssetManager::GetAssetAbsolutePath(metadata.FilePath);

		SceneSerializer serializer(asset);
		bool serializeResult = serializer.DeserializeFromFile(absolutePath);

		return serializeResult;
	}
}
