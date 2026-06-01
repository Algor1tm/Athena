#include "AssetRegistry.h"
#include "Athena/Asset/AssetManager.h"
#include "Athena/Project/Project.h"
#include "Athena/Core/YAMLTypes.h"
#include "Athena/Core/FileSystem.h"

#include <yaml-cpp/yaml.h>


namespace Athena
{
	AssetRegistry::AssetRegistry()
	{

	}

	void AssetRegistry::AddAsset(AssetHandle handle, const AssetMetadata& metadata)
	{
		m_Registry.insert({ handle, metadata });
	}

	void AssetRegistry::RemoveAsset(AssetHandle handle)
	{
		m_Registry.erase_if(handle, [](auto& element) { return true; });
	}

	void AssetRegistry::MoveAsset(AssetHandle handle, const FilePath& path)
	{
		if (!m_Registry.contains(handle) || FileSystem::Exists(path))
			return;

		m_Registry.modify_if(handle, [&path](std::pair<const AssetHandle, AssetMetadata>& element) 
		{
			AssetMetadata& metadata = element.second;

			if (metadata.IsMemoryOnly)
				return;

			FilePath current = FileSystem::GetWorkingDirectory();
			FilePath oldPath = current / AssetManager::GetAssetAbsolutePath(metadata.FilePath);
			FilePath newPath = path.is_absolute() ? path : current / path;

			std::filesystem::rename(oldPath, newPath);

			metadata.FilePath = AssetManager::GetAssetRelativePath(path);
		});
	}

	AssetMetadata AssetRegistry::GetMetadata(AssetHandle handle) const
	{
		AssetMetadata result;
		m_Registry.if_contains(handle, [&result] (const std::pair<AssetHandle, AssetMetadata>& element)
		{
			result = element.second;
		});

		return result;
	}

	bool AssetRegistry::IsAssetHandlePresent(AssetHandle handle) const
	{
		return m_Registry.contains(handle);
	}

	bool AssetRegistry::IsFilePathPresent(const FilePath& path) const
	{
		FilePath relPath = AssetManager::GetAssetRelativePath(path);
		bool result = false;

		m_Registry.for_each([&relPath, &result](const std::pair<AssetHandle, AssetMetadata>& element)
		{
			const auto& [handle, metadata] = element;

			if (metadata.FilePath == relPath)
				result = true;
		});

		return result;
	}

	AssetHandle AssetRegistry::GetAssetHandleFromFilePath(const FilePath& path) const
	{
		FilePath relPath = AssetManager::GetAssetRelativePath(path);
		relPath = FileSystem::GenericFormat(relPath);

		if (relPath.empty())
			return 0;

		AssetHandle result = 0;
		m_Registry.for_each([&relPath, &result](const std::pair<AssetHandle, AssetMetadata>& element)
		{
			const auto& [handle, metadata] = element;

			if (!metadata.IsMemoryOnly && metadata.FilePath == relPath)
				result = handle;
		});

		return result;
	}

	void AssetRegistry::Serialize()
	{
		auto path = Project::GetAssetRegistryPath();

		YAML::Emitter out;
		{
			out << YAML::BeginMap;
			out << YAML::Key << "AssetRegistry" << YAML::Value;

			out << YAML::BeginSeq;

			m_Registry.for_each([&out](const std::pair<AssetHandle, AssetMetadata>& element)
			{
				const auto& [handle, metadata] = element;

				if (metadata.IsMemoryOnly)
					return;

				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << handle;
				out << YAML::Key << "Type" << YAML::Value << AssetManager::AssetTypeToString(metadata.Type);
				out << YAML::Key << "FilePath" << YAML::Value << metadata.FilePath;
				out << YAML::EndMap;
			});

			out << YAML::EndSeq;
			out << YAML::EndMap;
		}

		std::ofstream fout(path);
		fout << out.c_str();
	}

	bool AssetRegistry::Deserialize()
	{
		auto path = Project::GetAssetRegistryPath();

		if (!FileSystem::Exists(path))
			FileSystem::WriteFile(path, 0, 0);

		YAML::Node data = YAML::TryLoadYAMLFile(path);

		if (!data)
		{
			return false;
		}

		auto rootNode = data["AssetRegistry"];
		if (!rootNode)
			return false;

		for (const auto& node : rootNode)
		{
			AssetHandle handle = YAML::TryReadYAMLValue<AssetHandle>(node, "Handle", 0);

			AssetMetadata metadata;
			metadata.Type = AssetManager::AssetTypeFromString(YAML::TryReadYAMLValue<String>(node, "Type", "Invalid"));
			metadata.FilePath = YAML::TryReadYAMLValue<String>(node, "FilePath", "Invalid");
			metadata.IsMemoryOnly = false;

			if (handle == 0 || metadata.Type == AssetType::None || !FileSystem::Exists(AssetManager::GetAssetAbsolutePath(metadata.FilePath)))
				continue;

			AddAsset(handle, metadata);
		}

		return true;
	}
}
