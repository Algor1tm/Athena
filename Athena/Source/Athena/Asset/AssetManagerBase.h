#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Asset/Asset.h"


namespace Athena
{
	class ATHENA_API AssetManagerBase
	{
	public:
		virtual ~AssetManagerBase() = default;

		virtual Ref<Asset> GetAsset(AssetHandle handle) = 0;

		virtual bool IsAssetHandleValid(AssetHandle handle) const = 0;
		virtual bool IsAssetLoaded(AssetHandle handle) const = 0;
		virtual const AssetMetadata& GetAssetMetadata(AssetHandle handle) const = 0;
		virtual const FilePath& GetAssetFilePath(AssetHandle handle) const = 0;
		virtual AssetType GetAssetType(AssetHandle handle) const = 0;
	};
}
