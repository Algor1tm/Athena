#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Thread.h"
#include "Athena/Asset/Asset.h"
#include "Athena/Asset/AssetRegistry.h"
#include "Athena/Asset/AssetImporter.h"
#include "Athena/Asset/AssetManagerBase.h"

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

		virtual AssetMetadata GetAssetMetadata(AssetHandle handle) const override;
		virtual FilePath GetAssetFilePath(AssetHandle handle) const override;
		virtual AssetType GetAssetType(AssetHandle handle) const override;

		AssetHandle AddMemoryOnlyAsset(const Ref<Asset>& asset);
		AssetHandle AddAsset(const Ref<Asset>& asset, const FilePath& path);
		void ReloadAsset(AssetHandle handle);
		void UnloadAsset(AssetHandle handle);

		void SerializeAllAssets();
		void DeserializeAllAssets() const;

		AssetHandle GetAssetHandleFromFilePath(const FilePath& filepath) const;
		AssetRegistry& GetAssetRegistry() { return m_AssetRegistry; };
		std::vector<String> GetAssetExtensions(AssetType type) const;

		Thread& GetAssetThread();

	private:
		AssetRegistry m_AssetRegistry;
		AssetImporter m_AssetImporter;

		ParallelFlatHashMap<AssetHandle, Ref<Asset>, 2> m_LoadedAssets;
	};
}
