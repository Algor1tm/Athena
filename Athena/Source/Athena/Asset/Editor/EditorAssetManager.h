#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Thread.h"
#include "Athena/Asset/Asset.h"
#include "Athena/Asset/AssetManagerBase.h"
#include "Athena/Asset/Editor/AssetImportSettings.h"
#include "Athena/Asset/Editor/AssetRegistry.h"
#include "Athena/Asset/Editor/AssetWatcherThread.h"

#include <unordered_map>


namespace Athena
{
	class ATHENA_API EditorAssetManager: public AssetManagerBase
	{
	public:
		EditorAssetManager();
		virtual ~EditorAssetManager();

		virtual Ref<Asset> GetAsset(AssetHandle handle) override;

		virtual bool IsAssetHandleValid(AssetHandle handle) const override;
		virtual bool IsAssetLoaded(AssetHandle handle) const override;
		bool IsAssetMemoryOnly(AssetHandle handle) const;

		virtual AssetMetadata GetAssetMetadata(AssetHandle handle) const override;
		virtual FilePath GetAssetFilePath(AssetHandle handle) const override;
		virtual AssetType GetAssetType(AssetHandle handle) const override;

		AssetHandle AddMemoryOnlyAsset(const Ref<Asset>& asset);
		AssetHandle AddAsset(const Ref<Asset>& asset, const FilePath& path);
		void ReloadAsset(AssetHandle handle);
		void UnloadAsset(AssetHandle handle);

		void SerializeAllAssets();
		void DeserializeAllAssets();

		Ref<AssetImportSettings> GetAssetImportSettings(AssetHandle handle);
		void SetAssetImportSettings(AssetHandle handle, const Ref<AssetImportSettings>& settings);
		void SerializeAssetImportSettings(AssetHandle handle);

		bool HasImportSettings(AssetType type);
		Ref<AssetImportSettings> GetDefaultImportSettings(AssetType type);

		AssetHandle GetAssetHandleFromFilePath(const FilePath& filepath) const;
		AssetRegistry& GetAssetRegistry() { return m_AssetRegistry; };

		Thread& GetAssetWatcherThread();

	private:
		Ref<Asset> LoadAsset(AssetHandle handle, const AssetMetadata& metadata);
		bool SerializeAsset(const Ref<Asset>& asset, const AssetMetadata& metadata);
		bool DeserializeAsset(const Ref<Asset>& asset, const AssetMetadata& metadata);

	private:
		AssetRegistry m_AssetRegistry;
		AssetWatcherThread m_AssetWatcherThread;

		ParallelFlatHashMap<AssetHandle, Ref<Asset>, 4> m_LoadedAssets;
		ParallelFlatHashMap<AssetHandle, AssetMetadata, 4> m_MemoryOnlyAssetsMetadata;

		ParallelFlatHashMap<AssetHandle, Ref<AssetImportSettings>, 4> m_LoadedAssetImportSettings;
		std::unordered_map<AssetType, Ref<AssetImportSettings>> m_DefaultSettingsMap;
	};
}
