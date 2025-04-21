#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Asset/Asset.h"

#include <unordered_map>
#include <mutex>


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

		std::unordered_map<AssetHandle, AssetMetadata> GetRegistryCopy() const;

	private:
		std::unordered_map<AssetHandle, AssetMetadata> m_Registry;
		mutable std::mutex m_Mutex;
	};
}
