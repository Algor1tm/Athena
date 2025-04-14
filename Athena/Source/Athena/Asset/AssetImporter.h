#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Thread.h"
#include "Athena/Asset/AssetRegistry.h"


namespace Athena
{
	class AssetImporter
	{
	public:
		AssetImporter();
		~AssetImporter();

		void Initialize(AssetRegistry* registry);

		Ref<Asset> LoadAsset(AssetHandle handle, const AssetMetadata& metadata);
		String GetAssetExtensions(AssetType type) const;

	private:
		void MonitorAssetsWrapper();
		void MonitorAssets();

	private:
		AssetRegistry* m_Registry = nullptr;
		Thread m_AssetThread;
		bool m_JoinAssetThread = false;
	};
}
