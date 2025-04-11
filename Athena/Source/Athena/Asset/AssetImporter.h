#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Asset/AssetRegistry.h"


namespace Athena
{
	// TODO: multithreading
	// 
	// Editor only
	// Responsible for automatically adding assets to asset registry 
	// and creating assets using EditorAssetLoader
	class AssetImporter
	{
	public:
		AssetImporter();
		~AssetImporter();

		void Initialize(AssetRegistry* registry);

		Ref<Asset> LoadAsset(AssetHandle handle, const AssetMetadata& metadata);
		void MonitorAssets();

		String GetAssetExtensions(AssetType type) const;

	private:
		AssetRegistry* m_Registry = nullptr;
	};
}
