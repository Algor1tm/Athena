#include "AssetFileExtensions.h"

namespace Athena
{
	std::unordered_map<FilePath, AssetType> AssetFileExtensions::m_AssetFileExtensionMap;
	String AssetFileExtensions::m_ImportSettingsExt;

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
			{ ".fbx",   AssetType::Mesh },
			{ ".gltf",  AssetType::Mesh },
			{ ".obj",   AssetType::Mesh },
			{ ".blend", AssetType::Mesh },
			{ ".x3d",   AssetType::Mesh },
			{ ".stl",   AssetType::Mesh },

			// Environment maps
			{ ".hdr", AssetType::EnvironmentMap },

			// Fonts
			{ ".ttf", AssetType::Font },
			{ ".otf", AssetType::Font },
			{ ".TTF", AssetType::Font },

			// Scenes
			{ ".atscene", AssetType::Scene },

			// Materials
			{ ".atmat", AssetType::Material}
		};

		m_ImportSettingsExt = ".import";
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

	FilePath AssetFileExtensions::GetImportSettingsPath(const FilePath& assetPath)
	{
		return FilePath(assetPath.string() + m_ImportSettingsExt);
	}
}
