#include "AssetImporter.h"
#include "Athena/Asset/AssetManager.h"
#include "Athena/Asset/TextureImporter.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Renderer/Font.h"
#include "Athena/Renderer/MaterialAsset.h"
#include "Athena/Scene/SceneSerializer.h"

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
		: m_AssetThread("AssetThread", [this]() { AssetThreadFunction(); })
	{
		m_Serializers[AssetType::Material] = Scope<MaterialSerializer>::Create();
		m_Serializers[AssetType::Scene] = Scope<SceneAssetSerializer>::Create();
	}

	AssetImporter::~AssetImporter()
	{
		m_JoinAssetThread = true;
		m_AssetThread.Join();
	}

	void AssetImporter::Initialize(AssetRegistry* registry)
	{
		m_Registry = registry;
		m_AssetThread.Start();
	}

	Ref<Asset> AssetImporter::LoadAsset(AssetHandle handle, const AssetMetadata& metadata)
	{
		ATN_PROFILE_FUNC();

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
			result = TextureImporter::Load(absolutePath, false);
		}

		if (assetType == AssetType::Material)
		{
			result = MaterialAsset::Create();
		}

		if (result)
		{
			result->Handle = handle;

			// Deserialize from file 
			if (m_Serializers.contains(assetType))
				m_Serializers.at(assetType)->TryLoadData(result, metadata);
		}


		return result;
	}

	void AssetImporter::SerializeAsset(const Ref<Asset>& asset, const AssetMetadata& metadata) const
	{
		if (m_Serializers.contains(metadata.Type))
		{
			m_Serializers.at(metadata.Type)->Serialize(asset, metadata);
		}
	}

	void AssetImporter::AssetThreadFunction()
	{
		while (!m_JoinAssetThread)
		{
			MonitorAssets();
			Thread::CurrentThreadSleep(Time::Seconds(2.f));
		}
	}

	// TODO: Check file timestamps for asset hot reloading
	void AssetImporter::MonitorAssets()
	{
		ATN_PROFILE_FUNC();

		// 1. Remove outdated or invalid assets
		auto registry = m_Registry->GetRegistryCopy();
		bool changed = false;

		for (const auto& [handle, meta] : registry)
		{
			if (!meta.IsMemoryOnly && !FileSystem::Exists(AssetManager::GetAssetAbsolutePath(meta.FilePath)))
			{
				m_Registry->RemoveAsset(handle);

				ATN_CORE_INFO_TAG("AssetManager", "(AssetThread) Deleting asset from asset registry (path - {}, type - {}, handle - {})", 
					meta.FilePath, Utils::AssetTypeToString(meta.Type), handle);
				changed = true;
			}

			if (meta.Type == AssetType::None)
			{
				m_Registry->RemoveAsset(handle);

				ATN_CORE_INFO_TAG("AssetManager", "(AssetThread) Deleting asset from asset registry (path - {}, type - {}, handle - {})",
					meta.FilePath, Utils::AssetTypeToString(meta.Type), handle);
				changed = true;
			}
		}


		// 2. Find new assets and import them
		FilePath assetDirectory = Project::GetAssetDirectory();

		std::queue<FilePath> queue;
		queue.push(assetDirectory);

		while (!queue.empty())
		{
			FilePath folderPath = queue.front();
			queue.pop();

			for (const auto& dirEntry : std::filesystem::directory_iterator(folderPath, std::filesystem::directory_options::skip_permission_denied))
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
					if (!m_Registry->IsFilePathPresent(path))
					{
						// Generate handle
						AssetHandle handle = AssetHandle();
						AssetMetadata metadata;
						metadata.FilePath = AssetManager::GetAssetRelativePath(path);
						metadata.Type = s_AssetExtensionMap.at(ext);
						metadata.IsMemoryOnly = false;

						m_Registry->AddAsset(handle, metadata);

						ATN_CORE_INFO_TAG("AssetManager", "(AssetThread) Adding new asset to asset registry (path - {}, type - {}, handle - {})",
							metadata.FilePath, Utils::AssetTypeToString(metadata.Type), handle);
						changed = true;
					}
				}
			}
		}

		if(changed)
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
