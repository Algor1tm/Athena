#pragma once

#include "Athena/Asset/AssetSerializers.h"
#include "Athena/Core/Core.h"
#include "Athena/Core/Thread.h"

#include <parallel_hashmap/phmap.h>

namespace Athena
{
	template <class K, class V, size_t N>
	using ParallelFlatHashMap = phmap::parallel_flat_hash_map<K, V, 
		phmap::priv::hash_default_hash<K>, 
		phmap::priv::hash_default_eq<K>, 
		phmap::priv::Allocator<phmap::priv::Pair<const K, V>>, 
		N, std::mutex>;

	class AssetRegistry;

	class AssetImporter
	{
	public:
		AssetImporter();
		~AssetImporter();

		void Initialize(AssetRegistry* registry);

		Ref<Asset> LoadAsset(AssetHandle handle, const AssetMetadata& metadata) const;
		void SerializeAsset(const Ref<Asset>& asset, const AssetMetadata& metadata);
		void DeserializeAsset(const Ref<Asset>& asset, const AssetMetadata& metadata) const;

		std::vector<String> GetAssetExtensions(AssetType type) const;
		Thread& GetAssetThread() { return m_AssetThread; }

	private:
		void AssetThreadFunction();
		void MonitorAssets();

	private:
		AssetRegistry* m_Registry = nullptr;
		std::unordered_map<AssetType, Ref<AssetSerializer>> m_Serializers;
		ParallelFlatHashMap<AssetHandle, uint64, 2> m_AssetsLastWriteTimeMap;

		Thread m_AssetThread;
		bool m_JoinAssetThread = false;
	};
}
