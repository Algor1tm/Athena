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
		std::lock_guard<std::mutex> lock(m_Mutex);

		m_Registry[handle] = metadata;
	}

	void AssetRegistry::RemoveAsset(AssetHandle handle)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_Registry.contains(handle))
			m_Registry.erase(handle);
	}

	const AssetMetadata& AssetRegistry::GetMetadata(AssetHandle handle) const
	{
		static const AssetMetadata s_NullMetadata;

		std::lock_guard<std::mutex> lock(m_Mutex);

		auto it = m_Registry.find(handle);
		if (it == m_Registry.end())
			return s_NullMetadata;

		return it->second;
	}

	bool AssetRegistry::IsAssetHandlePresent(AssetHandle handle) const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		return m_Registry.contains(handle);
	}

	bool AssetRegistry::IsFilePathPresent(const FilePath& path) const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		for (const auto& [handle, metadata] : m_Registry)
		{
			if (metadata.FilePath == path)
				return true;
		}

		return false;
	}

	AssetHandle AssetRegistry::GetAssetHandleFromFilePath(const FilePath& path) const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

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

	std::unordered_map<AssetHandle, AssetMetadata> AssetRegistry::GetRegistryCopy() const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		std::unordered_map<AssetHandle, AssetMetadata> copy = m_Registry;

		return copy;
	}

	void AssetRegistry::Serialize()
	{
		auto path = Project::GetAssetRegistryPath();

		YAML::Emitter out;
		{
			out << YAML::BeginMap;
			out << YAML::Key << "AssetRegistry" << YAML::Value;

			out << YAML::BeginSeq;

			std::lock_guard<std::mutex> lock(m_Mutex);

			for (const auto& [handle, metadata] : m_Registry)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << handle;
				out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(metadata.Type);
				out << YAML::Key << "FilePath" << YAML::Value << metadata.FilePath;
				//out << YAML::Key << "IsMemoryOnly" << YAML::Value << metadata.IsMemoryOnly;
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

		std::lock_guard<std::mutex> lock(m_Mutex);

		for (const auto& node : rootNode)
		{
			AssetHandle handle = node["Handle"].as<UUID>();
			auto& metadata = m_Registry[handle];
			metadata.Type = Utils::AssetTypeFromString(node["Type"].as<String>());
			metadata.FilePath = node["FilePath"].as<String>();
			metadata.IsMemoryOnly = false;//node["IsMemoryOnly"].as<bool>();
		}

		return true;
	}
}
