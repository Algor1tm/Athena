#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Asset/Asset.h"

#include <unordered_map>


namespace Athena
{
	class ATHENA_API AssetRegistry
	{
	public:
		AssetRegistry();

		void AddAsset(AssetHandle handle, const AssetMetadata& metadata);
		const AssetMetadata& GetMetadata(AssetHandle handle) const;

		bool IsAssetHandlePresent(AssetHandle handle) const;
		bool IsFilePathPresent(const FilePath& path) const;
		AssetHandle GetAssetHandleFromFilePath(const FilePath& path);

		void Serialize();
		bool Deserialize();

	private:
		std::unordered_map<AssetHandle, AssetMetadata> m_Registry;
	};
}
