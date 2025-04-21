#include "EditorAssetManager.h"
#include "Athena/Asset/AssetManager.h"
#include "Athena/Core/FileSystem.h"


namespace Athena
{
	EditorAssetManager::EditorAssetManager()
	{
		m_AssetRegistry.Deserialize();
		m_AssetImporter.Initialize(&m_AssetRegistry);
	}

	EditorAssetManager::~EditorAssetManager()
	{

	}

	WeakRef<Asset> EditorAssetManager::GetAsset(AssetHandle handle)
	{
		ATN_PROFILE_FUNC();

		if (!IsAssetHandleValid(handle))
			return nullptr;

		WeakRef<Asset> asset;
		if (IsAssetLoaded(handle))
		{
			asset = m_LoadedAssets.at(handle);
		}
		else
		{
			Ref<Asset> loadedAsset = m_AssetImporter.LoadAsset(handle, GetAssetMetadata(handle));
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
		m_LoadedAssets[handle] = asset;

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

		asset->Handle = handle;

		m_AssetRegistry.AddAsset(handle, metadata);
		m_AssetRegistry.Serialize();
		m_AssetImporter.SerializeAsset(asset, metadata);
		m_LoadedAssets[handle] = asset;

		return handle;
	}

	void EditorAssetManager::SaveAllAssets() const
	{
		for (const auto& [handle, asset] : m_LoadedAssets)
		{
			m_AssetImporter.SerializeAsset(asset, GetAssetMetadata(handle));
		}
	}

	Thread& EditorAssetManager::GetAssetThread()
	{
		return m_AssetImporter.GetAssetThread();
	}

	String EditorAssetManager::GetAssetExtensions(AssetType type) const
	{
		return m_AssetImporter.GetAssetExtensions(type);
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
