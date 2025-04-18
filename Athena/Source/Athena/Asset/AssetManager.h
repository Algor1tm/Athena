#pragma once

#include "AssetManagerBase.h"
#include "Athena/Project/Project.h"


namespace Athena
{
	class AssetManager
	{
	public:
		template<typename T>
		static WeakRef<T> GetAsset(AssetHandle handle)
		{
			WeakRef<Asset> asset = Project::GetActive()->GetAssetManager()->GetAsset(handle);
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

		static const AssetMetadata& GetAssetMetadata(AssetHandle handle)
		{
			return Project::GetActive()->GetAssetManager()->GetAssetMetadata(handle);
		}

		static const FilePath& GetAssetFilePath(AssetHandle handle)
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


	template <typename T>
	class AssetHandleRef
	{
	public:
		AssetHandleRef() = default;

		AssetHandleRef(AssetHandle handle)
			: m_Handle(handle)
		{
			
		}

		Ref<T> Get() const
		{
			if(IsExpired())
			{
				m_Asset = AssetManager::GetAsset<T>(m_Handle);
			}

			return m_Asset.Lock();
		}

		AssetHandle GetHandle() const
		{
			return m_Handle;
		}
			
		AssetType GetAssetType() const
		{
			if (IsExpired())
				return AssetType::None;

			return m_Asset->GetAssetType();
		}

		bool IsExpired() const
		{
			return m_Asset == nullptr || m_Asset.Expired();
		}

		operator AssetHandle() const { return m_Handle; }

		bool operator==(const AssetHandleRef& other) const
		{
			return m_Handle == other.m_Handle;
		}

		bool operator!=(const AssetHandleRef& other) const
		{
			return m_Handle != other.m_Handle;
		}

		explicit operator bool() const
		{
			return m_Handle != 0;
		}

	private:
		mutable WeakRef<T> m_Asset = nullptr;
		AssetHandle m_Handle = 0;
	};
}
