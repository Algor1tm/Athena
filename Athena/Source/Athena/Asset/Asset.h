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
		EnvironmentMap,
		Material,
		Font
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
		virtual AssetType GetType() const = 0;

		AssetHandle Handle;
	};


	namespace Utils
	{
		inline std::string_view AssetTypeToString(AssetType type)
		{
			switch (type)
			{
			case AssetType::None:			return "None";
			case AssetType::Texture2D:		return "Texture2D";
			case AssetType::EnvironmentMap: return "EnvironmentMap";
			case AssetType::Material:		return "Material";
			case AssetType::Font:			return "Font";
			}

			return "<Invalid>";
		}

		inline AssetType AssetTypeFromString(std::string_view assetType)
		{
			if (assetType == "None")			  return AssetType::None;
			if (assetType == "Texture2D")		  return AssetType::Texture2D;
			if (assetType == "EnvironmentMap")	  return AssetType::EnvironmentMap;
			if (assetType == "Material")		  return AssetType::Material;
			if (assetType == "Font")			  return AssetType::Font;

			return AssetType::None;
		}
	}
}
