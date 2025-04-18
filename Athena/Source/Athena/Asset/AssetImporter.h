#pragma once

#include "Athena/Asset/AssetRegistry.h"
#include "Athena/Asset/AssetSerializers.h"
#include "Athena/Core/Core.h"
#include "Athena/Core/Thread.h"


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
		void AssetThreadFunction();
		void MonitorAssets();

	private:
		AssetRegistry* m_Registry = nullptr;
		std::unordered_map<AssetType, Scope<AssetSerializer>> m_Serializers;

		Thread m_AssetThread;
		bool m_JoinAssetThread = false;
	};
}
