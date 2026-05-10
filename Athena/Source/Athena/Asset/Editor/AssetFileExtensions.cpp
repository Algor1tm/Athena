#include "AssetFileExtensions.h"

namespace Athena
{
	std::unordered_map<FilePath, AssetType> AssetFileExtensions::m_AssetFileExtensionMap;

	void AssetFileExtensions::Init()
	{
		// TODO: load from file

		m_AssetFileExtensionMap = {
			// Textures
			{ ".png",  AssetType::Texture },
			{ ".jpeg", AssetType::Texture },
			{ ".PIC",  AssetType::Texture },
			{ ".gif",  AssetType::Texture },
			{ ".tga",  AssetType::Texture },
			{ ".bmp",  AssetType::Texture },
			{ ".ppm",  AssetType::Texture },
			{ ".pgm",  AssetType::Texture },

			// Meshes 
			{ ".fbx",   AssetType::MeshSource },
			{ ".gltf",  AssetType::MeshSource },
			{ ".obj",   AssetType::MeshSource },
			{ ".blend", AssetType::MeshSource },
			{ ".x3d",   AssetType::MeshSource },
			{ ".stl",   AssetType::MeshSource },

			{ ".athsmesh",      AssetType::StaticMesh },
			{ ".athskelmesh",   AssetType::SkeletalMesh },

			// Environment maps
			{ ".hdr", AssetType::EnvironmentMap },

			// Fonts
			{ ".ttf", AssetType::Font },
			{ ".otf", AssetType::Font },
			{ ".TTF", AssetType::Font },

			// Scenes
			{ ".athscene", AssetType::Scene },

			// Materials
			{ ".athmat", AssetType::Material}
		};
	}

	std::vector<String> AssetFileExtensions::GetAssetExtensionsList(AssetType assetType)
	{
		std::vector<String> result;

		for (const auto& [ext, type] : m_AssetFileExtensionMap)
		{
			if (assetType == type)
			{
				result.push_back(ext.string());
			}
		}

		return result;
	}

	AssetType AssetFileExtensions::ExtensionToAssetType(const FilePath& ext)
	{
		if (m_AssetFileExtensionMap.contains(ext))
		{
			return m_AssetFileExtensionMap.at(ext);
		}

		return AssetType::None;
	}
}
