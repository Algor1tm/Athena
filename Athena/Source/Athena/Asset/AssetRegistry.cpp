#include "AssetRegistry.h"
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
		m_Registry[handle] = metadata;
	}

	const AssetMetadata& AssetRegistry::GetMetadata(AssetHandle handle) const
	{
		static const AssetMetadata s_NullMetadata;

		auto it = m_Registry.find(handle);
		if (it == m_Registry.end())
			return s_NullMetadata;

		return it->second;
	}

	bool AssetRegistry::IsAssetHandlePresent(AssetHandle handle) const
	{
		return m_Registry.contains(handle);
	}

	bool AssetRegistry::IsFilePathPresent(const FilePath& path) const
	{
		for (const auto& [handle, metadata] : m_Registry)
		{
			if (metadata.FilePath == path)
				return true;
		}

		return false;
	}

	AssetHandle AssetRegistry::GetAssetHandleFromFilePath(const FilePath& path) const
	{
		FilePath relPath = path;
		if (path.is_absolute())
			relPath = AssetManager::GetAssetRelativePath(path);

		if (relPath.empty())
			return 0;

		relPath = FileSystem::GenericFormat(relPath);

		for (const auto& [handle, metadata] : m_Registry)
		{
			if (metadata.FilePath == relPath && !metadata.IsMemoryOnly)
				return handle;
		}

		return 0;
	}

	void AssetRegistry::Serialize()
	{
		auto path = Project::GetAssetRegistryPath();

		YAML::Emitter out;
		{
			out << YAML::BeginMap;
			out << YAML::Key << "AssetRegistry" << YAML::Value;

			out << YAML::BeginSeq;
			for (const auto& [handle, metadata] : m_Registry)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << handle;
				out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(metadata.Type);
				out << YAML::Key << "FilePath" << YAML::Value << metadata.FilePath;
				out << YAML::Key << "IsMemoryOnly" << YAML::Value << metadata.IsMemoryOnly;
				out << YAML::EndMap;
			}
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
			auto& metadata = m_Registry[handle];
			metadata.Type = Utils::AssetTypeFromString(node["Type"].as<String>());
			metadata.FilePath = node["FilePath"].as<String>();
			metadata.IsMemoryOnly = node["IsMemoryOnly"].as<bool>();
		}

		return true;
	}
}
