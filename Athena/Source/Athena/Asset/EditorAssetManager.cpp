#include "EditorAssetManager.h"
#include "AssetImporter.h"


namespace Athena
{
	EditorAssetManager::EditorAssetManager()
	{
		m_AssetRegistry.Deserialize();
		m_Importer.Initialize(&m_AssetRegistry);

		// For now monitor assets only on initializaiton
		m_Importer.MonitorAssets();
	}

	EditorAssetManager::~EditorAssetManager()
	{

	}

	WeakRef<Asset> EditorAssetManager::GetAsset(AssetHandle handle)
	{
		if (!IsAssetHandleValid(handle))
			return nullptr;

		WeakRef<Asset> asset;
		if (IsAssetLoaded(handle))
		{
			asset = m_LoadedAssets.at(handle);
		}
		else
		{
			Ref<Asset> loadedAsset = m_Importer.LoadAsset(handle, GetAssetMetadata(handle));
			if (loadedAsset)
			{
				m_LoadedAssets[handle] = loadedAsset;
				asset = loadedAsset;
			}
			else
			{
				ATN_CORE_ERROR_TAG("AssetManager", "Failed to load asset, handle - {}!", handle);
			}
		}

		return asset;
	}

	AssetHandle EditorAssetManager::GetAssetHandleFromFilePath(const FilePath& filepath)
	{
		return m_AssetRegistry.GetAssetHandleFromFilePath(filepath);
	}

	bool EditorAssetManager::IsAssetHandleValid(AssetHandle handle) const
	{
		return handle != 0 && m_AssetRegistry.IsAssetHandlePresent(handle);
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle) const
	{
		return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
	}

	const AssetMetadata& EditorAssetManager::GetAssetMetadata(AssetHandle handle) const
	{
		return m_AssetRegistry.GetMetadata(handle);
	}

	const FilePath& EditorAssetManager::GetAssetFilePath(AssetHandle handle) const
	{
		return GetAssetMetadata(handle).FilePath;
	}

	AssetType EditorAssetManager::GetAssetType(AssetHandle handle) const
	{
		return GetAssetMetadata(handle).Type;
	}
}
