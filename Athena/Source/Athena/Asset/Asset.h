#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/UUID.h"

/*
	Asset - polymorphic base class -> TextureAsset, MaterialAsset ...
	AssetHandle - UUID for Asset
	AssetRef - AssetHandle + cached Asset
	AssetRegistry - database that stores all asset handles and its metadata 
*/


namespace Athena
{
	using AssetHandle = UUID;

	enum class AssetType
	{
		None = 0,
		Texture2D,
		StaticEnvironmentMap,
		Material,
		Font,
		Scene
	};

	struct AssetMetadata
	{
		AssetType Type = AssetType::None;
		FilePath FilePath;
		bool IsMemoryOnly = false;
	};

	class ATHENA_API Asset
	{
	public:
		virtual AssetType GetAssetType() const = 0;

		AssetHandle Handle = 0;
	};


	namespace Utils
	{
		inline std::string_view AssetTypeToString(AssetType type)
		{
			switch (type)
			{
			case AssetType::None:				  return "None";
			case AssetType::Texture2D:			  return "Texture2D";
			case AssetType::StaticEnvironmentMap: return "StaticEnvironmentMap";
			case AssetType::Material:			  return "Material";
			case AssetType::Font:				  return "Font";
			case AssetType::Scene:				  return "Scene";
			}

			return "<Invalid>";
		}

		inline AssetType AssetTypeFromString(std::string_view assetType)
		{
			if (assetType == "None")					  return AssetType::None;
			if (assetType == "Texture2D")				  return AssetType::Texture2D;
			if (assetType == "StaticEnvironmentMap")	  return AssetType::StaticEnvironmentMap;
			if (assetType == "Material")				  return AssetType::Material;
			if (assetType == "Font")					  return AssetType::Font;
			if (assetType == "Scene")					  return AssetType::Scene;

			return AssetType::None;
		}
	}
}
