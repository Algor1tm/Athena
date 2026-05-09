#include "EditorAssetManager.h"
#include "Athena/Asset/AssetManager.h"
#include "Athena/Asset/Editor/AssetFileExtensions.h"
#include "Athena/Core/FileSystem.h"


namespace Athena
{
	EditorAssetManager::EditorAssetManager()
	{
		AssetFileExtensions::Init();

		m_AssetRegistry.Deserialize();
		m_AssetWatcherThread.Initialize(&m_AssetRegistry);
	}

	EditorAssetManager::~EditorAssetManager()
	{

	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle handle)
	{
		ATN_PROFILE_FUNC();

		Ref<Asset> asset;

		bool isAssetLoaded = m_LoadedAssets.if_contains(handle, [&asset](const std::pair<AssetHandle, Ref<Asset>>& element) 
		{
			asset = element.second;
		});

		if(!isAssetLoaded && IsAssetHandleValid(handle))
		{
			Ref<Asset> loadedAsset = LoadAsset(handle, GetAssetMetadata(handle));
			if (loadedAsset)
			{
				m_LoadedAssets.insert({ handle, loadedAsset });
				asset = loadedAsset;
			}
			else
			{
				ATN_CORE_ERROR_TAG("AssetManager", "Failed to load asset, handle - {}!", handle);
			}
		}

		return asset;
	}

	AssetHandle EditorAssetManager::AddMemoryOnlyAsset(const Ref<Asset>& asset)
	{
		if (!asset)
			return AssetHandle(0);

		AssetHandle handle = AssetHandle();
		AssetMetadata metadata;
		metadata.IsMemoryOnly = true;
		metadata.Type = asset->GetAssetType();

		asset->Handle = handle;

		m_AssetRegistry.AddAsset(handle, metadata);
		m_LoadedAssets.insert({ handle, asset });

		return handle;
	}

	AssetHandle EditorAssetManager::AddAsset(const Ref<Asset>& asset, const FilePath& path)
	{
		if (!asset)
			return AssetHandle(0);

		if (FileSystem::Exists(path))
			return AssetHandle(0);

		AssetHandle handle = AssetHandle();
		AssetMetadata metadata;
		metadata.IsMemoryOnly = false;
		metadata.Type = asset->GetAssetType();
		metadata.FilePath = AssetManager::GetAssetRelativePath(path);

		if (FileSystem::Exists(metadata.FilePath))
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Asset path should be in child directory of asset registry directory!");
			return AssetHandle(0);
		}

		asset->Handle = handle;

		m_AssetRegistry.AddAsset(handle, metadata);
		m_AssetRegistry.Serialize();
		SerializeAsset(asset, metadata);
		m_LoadedAssets.insert({ handle, asset });

		return handle;
	}

	void EditorAssetManager::ReloadAsset(AssetHandle handle)
	{
		if (!IsAssetHandleValid(handle) || !IsAssetLoaded(handle))
			return;

		m_LoadedAssets.erase(handle);

		Ref<Asset> asset = LoadAsset(handle, GetAssetMetadata(handle));
		if (asset)
			m_LoadedAssets.insert({ handle, asset });
		else
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to reload asset, handle - {}!", handle);
	}

	void EditorAssetManager::UnloadAsset(AssetHandle handle)
	{
		if (!IsAssetHandleValid(handle) || !IsAssetLoaded(handle))
			return;

		m_LoadedAssets.erase(handle);
	}

	Ref<Asset> EditorAssetManager::LoadAsset(AssetHandle handle, const AssetMetadata& metadata) const
	{
		ATN_PROFILE_FUNC();

		FilePath absolutePath = AssetManager::GetAssetAbsolutePath(metadata.FilePath);
		Ref<Asset> asset = AssetManager::CreateEmptyAsset(metadata.Type);

		if (!asset)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to load asset (handle - {}, type - {})!", handle, metadata.Type, metadata.FilePath);
			return nullptr;
		}

		asset->Handle = handle;
		bool result = asset->Deserialize(absolutePath);

		if (!result)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to load asset (handle - {}, type - {}, filepath - {})!", handle, metadata.Type, metadata.FilePath);
			return nullptr;
		}

		return asset;
	}

	bool EditorAssetManager::SerializeAsset(const Ref<Asset>& asset, const AssetMetadata& metadata)
	{
		if (metadata.IsMemoryOnly)
			return true;

		FilePath absolutePath = AssetManager::GetAssetAbsolutePath(metadata.FilePath);

		if (FileSystem::Exists(absolutePath))
		{
			bool result = asset->Serialize(absolutePath);

			// TODO: in theory asset watcher can reload asset before this is called because file timestamp updated here
			m_AssetWatcherThread.OnAssetSerialize(asset->Handle, absolutePath);

			return result;
		}

		ATN_CORE_ERROR_TAG("AssetManager", "Failed to serialize asset : invalid filepath (handle - {}, type - {}, filepath - {})", asset->Handle, metadata.Type, metadata.FilePath);
		return false;
	}

	bool EditorAssetManager::DeserializeAsset(const Ref<Asset>& asset, const AssetMetadata& metadata) const
	{
		if (metadata.IsMemoryOnly)
			return true;

		FilePath absolutePath = AssetManager::GetAssetAbsolutePath(metadata.FilePath);

		if (FileSystem::Exists(absolutePath))
		{
			return asset->Deserialize(absolutePath);
		}

		ATN_CORE_ERROR_TAG("AssetManager", "Failed to deserialize asset : invalid filepath (handle - {}, type - {}, filepath - {})", asset->Handle, metadata.Type, metadata.FilePath);
		return false;
	}

	void EditorAssetManager::SerializeAllAssets()
	{
		m_LoadedAssets.for_each([this](const std::pair<AssetHandle, Ref<Asset>>& element)
		{
			const auto& [handle, asset] = element;
			SerializeAsset(asset, GetAssetMetadata(handle));
		});
	}

	void EditorAssetManager::DeserializeAllAssets() const
	{
		m_LoadedAssets.for_each([this](const std::pair<AssetHandle, Ref<Asset>>& element)
		{
			const auto& [handle, asset] = element;
			DeserializeAsset(asset, GetAssetMetadata(handle));
		});
	}

	Thread& EditorAssetManager::GetAssetWatcherThread()
	{
		return m_AssetWatcherThread.GetThread();
	}

	AssetHandle EditorAssetManager::GetAssetHandleFromFilePath(const FilePath& filepath) const
	{
		return m_AssetRegistry.GetAssetHandleFromFilePath(filepath);
	}

	bool EditorAssetManager::IsAssetHandleValid(AssetHandle handle) const
	{
		return handle != 0 && m_AssetRegistry.IsAssetHandlePresent(handle);
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle) const
	{
		return m_LoadedAssets.contains(handle);
	}

	AssetMetadata EditorAssetManager::GetAssetMetadata(AssetHandle handle) const
	{
		return m_AssetRegistry.GetMetadata(handle);
	}

	FilePath EditorAssetManager::GetAssetFilePath(AssetHandle handle) const
	{
		return GetAssetMetadata(handle).FilePath;
	}

	AssetType EditorAssetManager::GetAssetType(AssetHandle handle) const
	{
		return GetAssetMetadata(handle).Type;
	}
}
