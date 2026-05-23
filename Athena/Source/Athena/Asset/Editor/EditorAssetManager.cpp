#include "EditorAssetManager.h"
#include "Athena/Asset/AssetManager.h"
#include "Athena/Asset/Editor/AssetFileExtensions.h"
#include "Athena/Asset/Editor/MeshImporter.h"
#include "Athena/Core/FileSystem.h"


namespace Athena
{
	EditorAssetManager::EditorAssetManager()
	{
		AssetFileExtensions::Init();

		m_DefaultSettingsMap[AssetType::Mesh] = Ref<MeshImportSettings>::Create();

		m_AssetRegistry.Deserialize();
		m_AssetWatcherThread.Initialize(&m_AssetRegistry);
	}

	EditorAssetManager::~EditorAssetManager()
	{
		m_AssetWatcherThread.Shutdown();
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

		m_MemoryOnlyAssetsMetadata.insert({ handle, metadata });
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
		if (!IsAssetHandleValid(handle) || !IsAssetLoaded(handle) || IsAssetMemoryOnly(handle))
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

	Ref<Asset> EditorAssetManager::LoadAsset(AssetHandle handle, const AssetMetadata& metadata)
	{
		ATN_PROFILE_FUNC();

		if (IsAssetMemoryOnly(handle))
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Cant load memory only asset (handle - {}, type - {})!", handle, metadata.Type);
			return nullptr;
		}

		Ref<Asset> asset = AssetManager::CreateEmptyAsset(metadata.Type);

		if (!asset)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to load asset (handle - {}, type - {}, filepath - {})!", handle, metadata.Type, metadata.FilePath);
			return nullptr;
		}

		asset->Handle = handle;
		bool result = DeserializeAsset(asset, metadata);

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

		m_AssetWatcherThread.DisableTimestampsWatching();

		SerializeAssetImportSettings(asset->Handle);
		bool result = asset->Serialize(absolutePath);

		m_AssetWatcherThread.UpdateAssetTimestamp(asset->Handle, absolutePath);
		m_AssetWatcherThread.EnableWatchingTimestamps();

		return result;
	}

	bool EditorAssetManager::DeserializeAsset(const Ref<Asset>& asset, const AssetMetadata& metadata)
	{
		if (metadata.IsMemoryOnly)
			return true;

		FilePath absolutePath = AssetManager::GetAssetAbsolutePath(metadata.FilePath);

		if (FileSystem::Exists(absolutePath))
		{
			if (m_LoadedAssetImportSettings.contains(asset->Handle))
				m_LoadedAssetImportSettings.erase(asset->Handle);

			Ref<AssetImportSettings> importSettings = GetAssetImportSettings(asset->Handle);
			return asset->Deserialize(absolutePath, importSettings);
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

	void EditorAssetManager::DeserializeAllAssets()
	{
		m_LoadedAssets.for_each([this](const std::pair<AssetHandle, Ref<Asset>>& element)
		{
			const auto& [handle, asset] = element;
			DeserializeAsset(asset, GetAssetMetadata(handle));
		});
	}

	Ref<AssetImportSettings> EditorAssetManager::GetAssetImportSettings(AssetHandle handle)
	{
		const AssetMetadata& meta = GetAssetMetadata(handle);

		// 1. Check that this type has import settings
		if (!HasImportSettings(meta.Type) || !IsAssetHandleValid(handle))
			return nullptr;

		// 2. Look up if it is already loaded
		Ref<AssetImportSettings> settings;
		bool isAssetLoaded = m_LoadedAssetImportSettings.if_contains(handle, [&settings](const std::pair<AssetHandle, Ref<AssetImportSettings>>& element)
		{
			settings = element.second;
		});

		if (settings)
			return settings;

		// 3. Try to deserialize or return default (AssetWatcherThread should create and serialize default import settings)
		settings = GetDefaultImportSettings(meta.Type);
		FilePath absolutePath = AssetManager::GetAssetAbsolutePath(meta.FilePath);
		FilePath settingsFilePath = AssetFileExtensions::GetImportSettingsPath(absolutePath);

		if (FileSystem::Exists(settingsFilePath))
		{
			settings->Deserialize(settingsFilePath);
		}

		m_LoadedAssetImportSettings.insert({ handle, settings });
		return settings;
	}

	void EditorAssetManager::SetAssetImportSettings(AssetHandle handle, const Ref<AssetImportSettings>& settings)
	{
		const AssetMetadata& meta = GetAssetMetadata(handle);

		if (!HasImportSettings(meta.Type) || !IsAssetHandleValid(handle))
			return;

		m_LoadedAssetImportSettings.modify_if(handle, [settings](std::pair<const AssetHandle, Ref<AssetImportSettings>>& element)
		{
			element.second = settings;
		});
	}

	void EditorAssetManager::SerializeAssetImportSettings(AssetHandle handle)
	{
		if (m_LoadedAssetImportSettings.contains(handle))
		{
			Ref<AssetImportSettings> importSettings = GetAssetImportSettings(handle);

			FilePath path = AssetFileExtensions::GetImportSettingsPath(GetAssetFilePath(handle));
			importSettings->Serialize(path);
		}
	}

	bool EditorAssetManager::HasImportSettings(AssetType type)
	{
		return GetDefaultImportSettings(type) != nullptr;
	}

	Ref<AssetImportSettings> EditorAssetManager::GetDefaultImportSettings(AssetType type)
	{
		if (m_DefaultSettingsMap.contains(type))
			return m_DefaultSettingsMap.at(type);

		return nullptr;
	}

	AssetHandle EditorAssetManager::GetAssetHandleFromFilePath(const FilePath& filepath) const
	{
		return m_AssetRegistry.GetAssetHandleFromFilePath(filepath);
	}

	Thread& EditorAssetManager::GetAssetWatcherThread()
	{
		return m_AssetWatcherThread.GetThread();
	}

	bool EditorAssetManager::IsAssetHandleValid(AssetHandle handle) const
	{
		return handle != 0 && (m_AssetRegistry.IsAssetHandlePresent(handle) || m_MemoryOnlyAssetsMetadata.contains(handle));
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle) const
	{
		return m_LoadedAssets.contains(handle);
	}

	bool EditorAssetManager::IsAssetMemoryOnly(AssetHandle handle) const
	{
		return m_MemoryOnlyAssetsMetadata.contains(handle);
	}

	AssetMetadata EditorAssetManager::GetAssetMetadata(AssetHandle handle) const
	{
		if (IsAssetMemoryOnly(handle))
			return m_MemoryOnlyAssetsMetadata.at(handle);

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
