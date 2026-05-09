#include "AssetWatcherThread.h"
#include "Athena/Asset/AssetManager.h"
#include "Athena/Asset/Editor/AssetRegistry.h"
#include "Athena/Asset/Editor/AssetFileExtensions.h"
#include "Athena/Core/FileSystem.h"

#include <queue>


namespace Athena
{
	AssetWatcherThread::AssetWatcherThread()
		: m_AssetWatcherThread("AssetWatcherThread", [this]() { AssetThreadFunction(); })
	{

	}

	AssetWatcherThread::~AssetWatcherThread()
	{
		m_JoinThread = true;
		m_AssetWatcherThread.Join();
	}

	void AssetWatcherThread::Initialize(AssetRegistry* registry)
	{
		m_Registry = registry;
		m_AssetWatcherThread.Start();
	}

	void AssetWatcherThread::OnAssetSerialize(AssetHandle handle, const FilePath& absolutePath)
	{
		FilePath absolutePathCopy = absolutePath;
		m_AssetsLastWriteTimeMap.modify_if(handle, [absolutePathCopy](std::pair<const AssetHandle, uint64>& element)
			{
				element.second = FileSystem::GetLastWriteTimestamp(absolutePathCopy);
			});
	}

	void AssetWatcherThread::AssetThreadFunction()
	{
		while (!m_JoinThread)
		{
			MonitorAssets();
			Thread::CurrentThreadSleep(Time::Seconds(MONITOR_SECONDS_INTERVAL));
		}
	}

	void AssetWatcherThread::MonitorAssets()
	{
		ATN_PROFILE_FUNC();

		// 1. Remove outdated or invalid assets and hot reload assets by last write time
		const auto& registry = m_Registry->GetRegistry();
		std::vector<AssetHandle> assetsToRemove;
		std::vector<AssetHandle> assetsToReload;

		registry.for_each([&assetsToRemove, &assetsToReload, this](const std::pair<AssetHandle, AssetMetadata>& element)
		{
			const auto& [handle, meta] = element;
			FilePath absolutePath = AssetManager::GetAssetAbsolutePath(meta.FilePath);

			if (!meta.IsMemoryOnly && !FileSystem::Exists(absolutePath))
			{
				assetsToRemove.push_back(handle);
			}
			else if (meta.Type == AssetType::None)
			{
				assetsToRemove.push_back(handle);
			}

			else if (!meta.IsMemoryOnly)
			{
				uint64 timestamp = FileSystem::GetLastWriteTimestamp(absolutePath);

				// Check old timestamp or emplace new if does not contain handle
				m_AssetsLastWriteTimeMap.try_emplace_l(handle, [timestamp, &assetsToReload](std::pair<const AssetHandle, uint64>& element)
					{
						auto& [handle, oldTimestamp] = element;

						if (oldTimestamp != timestamp)
						{
							assetsToReload.push_back(handle);
							oldTimestamp = timestamp;
						}
					}, timestamp);
			}
		});

		bool serialize = !assetsToRemove.empty();

		for (AssetHandle handle : assetsToRemove)
		{
			AssetMetadata meta = m_Registry->GetMetadata(handle);

			if (Project::GetEditorAssetManager()->IsAssetLoaded(handle))
				Project::GetEditorAssetManager()->UnloadAsset(handle);

			m_Registry->RemoveAsset(handle);
			m_AssetsLastWriteTimeMap.erase_if(handle, [](auto&) { return true; });

			ATN_CORE_INFO_TAG("AssetManager", "(AssetObserverThread) Deleting asset from asset registry (path - {}, type - {}, handle - {})",
				meta.FilePath, AssetManager::AssetTypeToString(meta.Type), handle);
		}

		for (AssetHandle handle : assetsToReload)
		{
			if (Project::GetEditorAssetManager()->IsAssetLoaded(handle))
			{
				Project::GetEditorAssetManager()->ReloadAsset(handle);

				AssetMetadata meta = AssetManager::GetAssetMetadata(handle);
				ATN_CORE_INFO_TAG("AssetManager", "(AssetObserverThread) Reloading asset (path - {}, type - {}, handle - {})",
					meta.FilePath, AssetManager::AssetTypeToString(meta.Type), handle);
			}
		}

		// 2. Find new assets and import them
		FilePath assetDirectory = Project::GetAssetDirectory();

		std::queue<FilePath> queue;
		queue.push(assetDirectory);

		while (!queue.empty())
		{
			FilePath folderPath = queue.front();
			queue.pop();

			for (const auto& dirEntry : std::filesystem::directory_iterator(folderPath, std::filesystem::directory_options::skip_permission_denied))
			{
				bool isFolder = dirEntry.is_directory();
				FilePath path = dirEntry.path();
				FilePath ext = path.extension();

				if (isFolder)
				{
					queue.push(path);
					continue;
				}

				if (AssetFileExtensions::ExtensionToAssetType(ext) == AssetType::None)
				{
					continue;
				}

				if (!m_Registry->IsFilePathPresent(path))
				{
					// Generate handle
					AssetHandle handle = AssetHandle();
					AssetMetadata metadata;
					metadata.FilePath = AssetManager::GetAssetRelativePath(path);
					metadata.Type = AssetFileExtensions::ExtensionToAssetType(ext);
					metadata.IsMemoryOnly = false;

					m_Registry->AddAsset(handle, metadata);
					m_AssetsLastWriteTimeMap.insert({ handle, FileSystem::GetLastWriteTimestamp(path) });

					ATN_CORE_INFO_TAG("AssetManager", "(AssetObserverThread) Adding new asset to asset registry (path - {}, type - {}, handle - {})",
						metadata.FilePath, AssetManager::AssetTypeToString(metadata.Type), handle);

					serialize = true;
				}
			}
		}

		if (serialize)
			m_Registry->Serialize();
	}


}
