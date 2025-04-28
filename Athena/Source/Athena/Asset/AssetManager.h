#pragma once

#include "AssetManagerBase.h"
#include "Athena/Project/Project.h"


namespace Athena
{
	class AssetManager
	{
	public:
		template<typename T>
		static Ref<T> GetAsset(AssetHandle handle)
		{
			Ref<Asset> asset = Project::GetActive()->GetAssetManager()->GetAsset(handle);
			return asset.As<T>();
		}

		static bool IsAssetHandleValid(AssetHandle handle)
		{
			return Project::GetActive()->GetAssetManager()->IsAssetHandleValid(handle);
		}

		static bool IsAssetLoaded(AssetHandle handle)
		{
			return Project::GetActive()->GetAssetManager()->IsAssetLoaded(handle);
		}

		static AssetMetadata GetAssetMetadata(AssetHandle handle)
		{
			return Project::GetActive()->GetAssetManager()->GetAssetMetadata(handle);
		}

		static FilePath GetAssetFilePath(AssetHandle handle)
		{
			return Project::GetActive()->GetAssetManager()->GetAssetFilePath(handle);
		}

		static AssetType GetAssetType(AssetHandle handle)
		{
			return Project::GetActive()->GetAssetManager()->GetAssetType(handle);
		}

		static FilePath GetAssetRelativePath(const FilePath& path)
		{
			return std::filesystem::relative(path, Project::GetAssetDirectory());
		}

		static FilePath GetAssetAbsolutePath(const FilePath& path)
		{
			return Project::GetAssetDirectory() / path;
		}
	};
}
