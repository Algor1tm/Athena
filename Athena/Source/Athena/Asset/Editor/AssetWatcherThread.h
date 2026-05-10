#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Asset/Asset.h"
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

	class AssetWatcherThread
	{
	public:
		AssetWatcherThread();
		~AssetWatcherThread();

		void Initialize(AssetRegistry* registry);
		void Shutdown();

		void OnAssetSerialize(AssetHandle handle, const FilePath& absolutePath);

		Thread& GetThread() { return m_AssetWatcherThread; }

	private:
		void AssetWatcherThreadFunction();
		void MonitorAssets();

	private:
		AssetRegistry* m_Registry = nullptr;
		ParallelFlatHashMap<AssetHandle, uint64, 2> m_AssetsLastWriteTimeMap;

		Thread m_AssetWatcherThread;
		std::atomic<bool> m_JoinThread;

		const float MONITOR_SECONDS_INTERVAL = 2.0f;
	};
}
