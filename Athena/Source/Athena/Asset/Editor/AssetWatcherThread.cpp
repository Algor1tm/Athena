#include "AssetWatcherThread.h"
#include "Athena/Asset/AssetManager.h"
#include "Athena/Asset/Editor/AssetRegistry.h"
#include "Athena/Asset/Editor/AssetFileExtensions.h"
#include "Athena/Core/FileSystem.h"

#include <queue>


namespace Athena
{
	AssetWatcherThread::AssetWatcherThread()
		: m_AssetWatcherThread("AssetWatcherThread", [this]() { AssetWatcherThreadFunction(); })
	{
		
	}

	AssetWatcherThread::~AssetWatcherThread()
	{

	}

	void AssetWatcherThread::Initialize(AssetRegistry* registry)
	{
		m_Registry = registry;

		m_JoinThread.store(false, std::memory_order_relaxed);
		m_WatchTimestamps.store(true);

		m_AssetWatcherThread.Start();
	}

	void AssetWatcherThread::Shutdown()
	{
		m_JoinThread.store(true, std::memory_order_relaxed);
		m_AssetWatcherThread.Join();
	}

	void AssetWatcherThread::DisableTimestampsWatching()
	{
		m_WatchTimestamps.store(false);
	}

	void AssetWatcherThread::EnableWatchingTimestamps()
	{
		m_WatchTimestamps.store(true);
	}

	void AssetWatcherThread::UpdateAssetTimestamp(AssetHandle handle, const FilePath& absolutePath)
	{
		FilePath absolutePathCopy = absolutePath;
		m_AssetsLastWriteTimeMap.modify_if(handle, [absolutePathCopy](std::pair<const AssetHandle, uint64>& element)
		{
			element.second = FileSystem::GetLastWriteTimestamp(absolutePathCopy);
		});
	}

	void AssetWatcherThread::AssetWatcherThreadFunction()
	{
		while (m_JoinThread.load(std::memory_order_relaxed) == false)
		{
			MonitorAssets();
			Thread::CurrentThreadSleep(Time::Seconds(MONITOR_INTERVAL_SECONDS));
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

			bool hasImportSettings = Project::GetEditorAssetManager()->HasImportSettings(meta.Type);
			FilePath importSettingsPath = AssetFileExtensions::GetImportSettingsPath(absolutePath);

			if (!FileSystem::Exists(absolutePath))
			{
				assetsToRemove.push_back(handle);
				return;
			}
			else if (meta.Type == AssetType::None)
			{
				assetsToRemove.push_back(handle);
				return;
			}
			else if (meta.IsMemoryOnly == true)
			{
				assetsToRemove.push_back(handle);
				return;
			}

			if (m_WatchTimestamps.load())
			{
				uint64 timestamp = FileSystem::GetLastWriteTimestamp(absolutePath);

				// Get Max timestamp from asset timestamp and importsettings timestamp
				if (hasImportSettings && FileSystem::Exists(importSettingsPath))
				{
					uint64 settingsTimestamp = FileSystem::GetLastWriteTimestamp(importSettingsPath);
					timestamp = Math::Max(timestamp, settingsTimestamp); 
				}

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

			if (hasImportSettings && !FileSystem::Exists(importSettingsPath))
			{
				Ref<AssetImportSettings> defaultSettings = Project::GetEditorAssetManager()->GetDefaultImportSettings(meta.Type);
				defaultSettings->Serialize(importSettingsPath);
				UpdateAssetTimestamp(handle, importSettingsPath);

				ATN_CORE_TRACE_TAG("AssetManager", "(AssetWatcherThread) Created import settings file for asset (path - {}, type - {}, handle - {})", importSettingsPath, meta.Type, handle);
			}
		});

		bool serialize = !assetsToRemove.empty();

		for (AssetHandle handle : assetsToRemove)
		{
			AssetMetadata meta = m_Registry->GetMetadata(handle);

			if (Project::GetEditorAssetManager()->IsAssetLoaded(handle))
			{
				Project::GetEditorAssetManager()->UnloadAsset(handle);
			}

			if (Project::GetEditorAssetManager()->HasImportSettings(meta.Type))
			{
				FilePath absolutePath = AssetManager::GetAssetAbsolutePath(meta.FilePath);
				FilePath importSettingsPath = AssetFileExtensions::GetImportSettingsPath(absolutePath);

				if (FileSystem::Exists(importSettingsPath))
				{
					FileSystem::Remove(importSettingsPath);

					ATN_CORE_TRACE_TAG("AssetManager", "(AssetWatcherThread) Deleted import settings file for asset (path - {}, type - {}, handle - {})", importSettingsPath, meta.Type, handle);
				}
			}

			m_Registry->RemoveAsset(handle);
			m_AssetsLastWriteTimeMap.erase_if(handle, [](auto&) { return true; });

			ATN_CORE_INFO_TAG("AssetManager", "(AssetWatcherThread) Deleted asset from asset registry (path - {}, type - {}, handle - {})",
				meta.FilePath, AssetManager::AssetTypeToString(meta.Type), handle);
		}

		for (AssetHandle handle : assetsToReload)
		{
			if (Project::GetEditorAssetManager()->IsAssetLoaded(handle))
			{
				Project::GetEditorAssetManager()->ReloadAsset(handle);

				AssetMetadata meta = AssetManager::GetAssetMetadata(handle);
				ATN_CORE_INFO_TAG("AssetManager", "(AssetWatcherThread) Reloaded asset (path - {}, type - {}, handle - {})",
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

					ATN_CORE_INFO_TAG("AssetManager", "(AssetWatcherThread) Added new asset to asset registry (path - {}, type - {}, handle - {})",
						metadata.FilePath, AssetManager::AssetTypeToString(metadata.Type), handle);

					serialize = true;
				}
			}
		}

		if (serialize)
			m_Registry->Serialize();
	}
}
