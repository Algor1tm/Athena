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
		m_Serializers[AssetType::Material] = Ref<MaterialSerializer>::Create();
		m_Serializers[AssetType::Scene] = Ref<SceneAssetSerializer>::Create();
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

	Ref<Asset> AssetImporter::LoadAsset(AssetHandle handle, const AssetMetadata& metadata) const
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
			DeserializeAsset(result, metadata);
		}

		return result;
	}

	void AssetImporter::SerializeAsset(const Ref<Asset>& asset, const AssetMetadata& metadata)
	{
		if (m_Serializers.contains(metadata.Type) && !metadata.IsMemoryOnly)
		{
			m_Serializers.at(metadata.Type)->Serialize(asset, metadata);

			m_AssetsLastWriteTimeMap.modify_if(asset->Handle, [&metadata](std::pair<const AssetHandle, uint64>& element) 
			{
				element.second = FileSystem::GetLastWriteTimestamp(AssetManager::GetAssetAbsolutePath(metadata.FilePath));
			});
		}
	}

	void AssetImporter::DeserializeAsset(const Ref<Asset>& asset, const AssetMetadata& metadata) const
	{
		if (m_Serializers.contains(metadata.Type) && !metadata.IsMemoryOnly)
		{
			m_Serializers.at(metadata.Type)->TryLoadData(asset, metadata);
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

	void AssetImporter::MonitorAssets()
	{
		ATN_PROFILE_FUNC();

		// 1. Remove outdated or invalid assets and hot reload assets by last write time
		const auto& registry = m_Registry->GetRegistry();
		std::vector<AssetHandle> assetsToRemove;
		std::vector<AssetHandle> assetsToReload;

		registry.for_each([&assetsToRemove, &assetsToReload, this](const std::pair<AssetHandle, AssetMetadata>& element)
		{
			const auto& [handle, meta] = element;
			FilePath absolutePath = AssetManager::GetAssetAbsolutePath(meta.FilePath);

			if (!meta.IsMemoryOnly && !FileSystem::Exists(absolutePath))
			{
				assetsToRemove.push_back(handle);
			}
			else if (meta.Type == AssetType::None)
			{
				assetsToRemove.push_back(handle);
			}

			else if (!meta.IsMemoryOnly)
			{
				uint64 timestamp = FileSystem::GetLastWriteTimestamp(absolutePath);

				// Check old timestamp or emplace new if does not contain handle
				m_AssetsLastWriteTimeMap.try_emplace_l(handle, [timestamp, &assetsToReload](std::pair<const AssetHandle, uint64>& element)
				{
					auto& [handle, oldTimestamp] = element;

					if (oldTimestamp != timestamp)
					{
						assetsToReload.push_back(handle);
						oldTimestamp = timestamp;
					}
				}, timestamp);
			}
		});

		bool serialize = !assetsToRemove.empty();

		for (AssetHandle handle : assetsToRemove)
		{
			AssetMetadata meta = m_Registry->GetMetadata(handle);

			if (Project::GetEditorAssetManager()->IsAssetLoaded(handle))
				Project::GetEditorAssetManager()->UnloadAsset(handle);

			m_Registry->RemoveAsset(handle);
			m_AssetsLastWriteTimeMap.erase_if(handle, [](auto&) { return true; });

			ATN_CORE_INFO_TAG("AssetManager", "(AssetThread) Deleting asset from asset registry (path - {}, type - {}, handle - {})",
				meta.FilePath, Utils::AssetTypeToString(meta.Type), handle);
		}

		for (AssetHandle handle : assetsToReload)
		{
			if (Project::GetEditorAssetManager()->IsAssetLoaded(handle))
			{
				Project::GetEditorAssetManager()->ReloadAsset(handle);

				const AssetMetadata& meta = AssetManager::GetAssetMetadata(handle);
				ATN_CORE_INFO_TAG("AssetManager", "(AssetThread) Reloading asset (path - {}, type - {}, handle - {})",
					meta.FilePath, Utils::AssetTypeToString(meta.Type), handle);
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

				if (!s_AssetExtensionMap.contains(ext))
				{
					continue;
				}

				if (!m_Registry->IsFilePathPresent(path))
				{
					// Generate handle
					AssetHandle handle = AssetHandle();
					AssetMetadata metadata;
					metadata.FilePath = AssetManager::GetAssetRelativePath(path);
					metadata.Type = s_AssetExtensionMap.at(ext);
					metadata.IsMemoryOnly = false;

					m_Registry->AddAsset(handle, metadata);
					m_AssetsLastWriteTimeMap.insert({ handle, FileSystem::GetLastWriteTimestamp(path) });

					ATN_CORE_INFO_TAG("AssetManager", "(AssetThread) Adding new asset to asset registry (path - {}, type - {}, handle - {})",
						metadata.FilePath, Utils::AssetTypeToString(metadata.Type), handle);

					serialize = true;
				}
			}
		}

		if(serialize)
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
