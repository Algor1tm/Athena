#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Asset/Asset.h"

#include <parallel_hashmap/phmap.h>


namespace Athena
{
	class ATHENA_API AssetRegistry
	{
	public:
		AssetRegistry();

		void AddAsset(AssetHandle handle, const AssetMetadata& metadata);
		void RemoveAsset(AssetHandle handle);
		void MoveAsset(AssetHandle handle, const FilePath& newPath);

		const AssetMetadata& GetMetadata(AssetHandle handle) const;

		bool IsAssetHandlePresent(AssetHandle handle) const;
		bool IsFilePathPresent(const FilePath& path) const;
		AssetHandle GetAssetHandleFromFilePath(const FilePath& path) const;

		void Serialize();
		bool Deserialize();

		const auto& GetRegistry() const { return m_Registry; }

	private:
		phmap::parallel_node_hash_map_m<AssetHandle, AssetMetadata> m_Registry;
	};
}
