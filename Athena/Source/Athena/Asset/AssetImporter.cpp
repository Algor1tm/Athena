#include "AssetImporter.h"
#include "Athena/Asset/AssetManager.h"

#include "Athena/Asset/TextureImporter.h"
#include "Athena/Scene/SceneSerializer.h"
#include "Athena/Renderer/Font.h"

#include <queue>

namespace Athena
{
	static std::unordered_map<FilePath, AssetType> s_AssetExtensionMap = {
		// Textures
		{ ".png",  AssetType::Texture2D },
		{ ".jpg",  AssetType::Texture2D },
		{ ".jpeg", AssetType::Texture2D },

		// Meshes 
		{ ".fbx",   AssetType::StaticMesh },
		{ ".gltf",  AssetType::StaticMesh },
		{ ".obj",   AssetType::StaticMesh },
		{ ".blend", AssetType::StaticMesh },
		{ ".x3d",   AssetType::StaticMesh },

		// Environment maps
		{ ".hdr", AssetType::EnvironmentMap },

		// Fonts
		{ ".ttf", AssetType::Font },
		{ ".TTF", AssetType::Font },

		// Scenes
		{ ".athscene", AssetType::Scene },

		// Materials
		{ ".athmat", AssetType::Material}
	};

	AssetImporter::AssetImporter()
	{

	}

	AssetImporter::~AssetImporter()
	{

	}

	void AssetImporter::Initialize(AssetRegistry* registry)
	{
		m_Registry = registry;
	}

	Ref<Asset> AssetImporter::LoadAsset(AssetHandle handle, const AssetMetadata& metadata)
	{
		Ref<Asset> result;
		AssetType assetType = metadata.Type;

		if (assetType == AssetType::None)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Asset invalid type - None, handle - {}!", handle);
			return result;
		}

		FilePath absolutePath = AssetManager::GetAssetAbsolutePath(metadata.FilePath);

		// TODO: create assets in more generic way
		if (assetType == AssetType::Font)
		{
			result = Font::Create(absolutePath);
		}

		if (assetType == AssetType::Scene)
		{
			result = Ref<Scene>::Create();

			SceneSerializer serializer(result);
			bool serializeResult = serializer.DeserializeFromFile(absolutePath);
		}

		if (assetType == AssetType::EnvironmentMap)
		{
			result = Ref<StaticEnvironmentMap>::Create(absolutePath);
		}

		if (assetType == AssetType::Texture2D)
		{
			result = TextureImporter::Load(absolutePath, true);
		}

		if (result)
			result->Handle = handle;

		return result;
	}

	// TODO:
	// 1. Run this function on different thread
	// 2. Check file timestamps to know if file is updated
	// 3. Delete outdated assets from registry
	void AssetImporter::MonitorAssets()
	{
		FilePath assetDirectory = Project::GetAssetDirectory();

		std::queue<FilePath> queue;
		queue.push(assetDirectory);

		while (!queue.empty())
		{
			FilePath folderPath = queue.front();
			queue.pop();

			for (const auto& dirEntry : std::filesystem::directory_iterator(folderPath))
			{
				bool isFolder = dirEntry.is_directory();
				FilePath path = dirEntry.path();
				FilePath ext = path.extension();

				if (isFolder)
				{
					queue.push(path);
					continue;
				}

				if (s_AssetExtensionMap.contains(ext))
				{
					path = AssetManager::GetAssetRelativePath(path);

					if (!m_Registry->IsFilePathPresent(path))
					{
						// Generate handle
						AssetHandle handle = AssetHandle();
						AssetMetadata metadata;
						metadata.FilePath = path;
						metadata.Type = s_AssetExtensionMap.at(ext);
						metadata.IsMemoryOnly = false;

						m_Registry->AddAsset(handle, metadata);
					}
				}
			}
		}

		m_Registry->Serialize();
	}

	String AssetImporter::GetAssetExtensions(AssetType assetType) const
	{
		String result;

		for (const auto& [ext, type] : s_AssetExtensionMap)
		{
			if (assetType == type)
			{
				result += fmt::format("*{} ", ext.string());
			}
		}

		// remove last space
		result.erase(result.end() - 1);
		return result;
	}
}
