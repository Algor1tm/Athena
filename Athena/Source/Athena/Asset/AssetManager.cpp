#include "AssetManager.h"
#include "Athena/Renderer/Font.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Mesh.h"
#include "Athena/Renderer/EnvironmentMap.h"
#include "Athena/Scene/Scene.h"


namespace Athena
{
	Ref<Asset> AssetManager::CreateEmptyAsset(AssetType assetType)
	{
		switch (assetType)
		{
		case AssetType::Texture:			  return Ref<TextureAsset>::Create();
		case AssetType::EnvironmentMap:		  return Ref<StaticEnvironmentMap>::Create();
		case AssetType::Material:			  return Ref<MaterialAsset>::Create();
		case AssetType::Font:				  return Ref<Font>::Create();
		case AssetType::Scene:				  return Ref<Scene>::Create();
		case AssetType::MeshSource:			  return Ref<MeshSource>::Create();
		case AssetType::StaticMesh:			  return Ref<StaticMesh>::Create();
		case AssetType::SkeletalMesh:		  return Ref<SkeletalMesh>::Create();
		}

		ATN_CORE_ASSERT(false, "Invalid asset type!");
		return nullptr;
	}

	std::string_view AssetManager::AssetTypeToString(AssetType type)
	{
		switch (type)
		{
		case AssetType::None:				  return "None";
		case AssetType::Texture:			  return "Texture";
		case AssetType::EnvironmentMap:		  return "EnvironmentMap";
		case AssetType::Material:			  return "Material";
		case AssetType::Font:				  return "Font";
		case AssetType::Scene:				  return "Scene";
		case AssetType::MeshSource:			  return "MeshSource";
		case AssetType::StaticMesh:			  return "StaticMesh";
		case AssetType::SkeletalMesh:		  return "SkeletalMesh";
		}

		ATN_CORE_ASSERT(false, "Invalid asset type!");
		return "<Invalid>";
	}

	AssetType AssetManager::AssetTypeFromString(std::string_view assetType)
	{
		if (assetType == "None")			  return AssetType::None;
		if (assetType == "Texture")			  return AssetType::Texture;
		if (assetType == "EnvironmentMap")	  return AssetType::EnvironmentMap;
		if (assetType == "Material")		  return AssetType::Material;
		if (assetType == "Font")			  return AssetType::Font;
		if (assetType == "Scene")			  return AssetType::Scene;
		if (assetType == "MeshSource")		  return AssetType::MeshSource;
		if (assetType == "StaticMesh")		  return AssetType::StaticMesh;
		if (assetType == "SkeletalMesh")	  return AssetType::SkeletalMesh;

		ATN_CORE_ASSERT(false, "Invalid asset type string!");
		return AssetType::None;
	}
}
