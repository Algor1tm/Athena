#include "AssetRegistry.h"
#include "Athena/Asset/AssetManager.h"
#include "Athena/Project/Project.h"
#include "Athena/Core/YAMLTypes.h"
#include "Athena/Core/FileSystem.h"
#include "AssetManager.h"

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

	const AssetMetadata& AssetRegistry::GetMetadata(AssetHandle handle) const
	{
		static const AssetMetadata s_NullMetadata;

#if 1
		if(m_Registry.contains(handle));
			return m_Registry.at(handle);

		return s_NullMetadata;
#else
		const AssetMetadata* result = &s_NullMetadata;
		m_Registry.if_contains(handle, [&result](const std::pair<AssetHandle, AssetMetadata>& element)
		{
			result = &element.second;
		});

		return *result;
#endif
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
				out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(metadata.Type);
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

		YAML::Node data;
		try
		{
			data = YAML::LoadFile(path.string());
		}
		catch (YAML::ParserException e)
		{
			ATN_CORE_ERROR("Failed to load asset registry file '{0}'\n     {1}", path, e.what());
			return false;
		}

		auto rootNode = data["AssetRegistry"];
		if (!rootNode)
			return false;

		for (const auto& node : rootNode)
		{
			AssetHandle handle = node["Handle"].as<UUID>();

			AssetMetadata metadata;
			metadata.Type = Utils::AssetTypeFromString(node["Type"].as<String>());
			metadata.FilePath = node["FilePath"].as<String>();
			metadata.IsMemoryOnly = false;

			AddAsset(handle, metadata);
		}

		return true;
	}
}
