#include "Mesh.h"

#include "Athena/Asset/AssetManager.h"
#include "Athena/Asset/Editor/MeshSourceImporter.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Project/Project.h"
#include "Athena/Renderer/Renderer.h"

#include "Athena/Core/YAMLTypes.h"


namespace Athena
{
	bool MeshSource::HasAnimation(const Ref<Animation>& animation) const
	{
		return std::find(m_Animations.begin(), m_Animations.end(), animation) != m_Animations.end();
	}

	bool MeshSource::Serialize(const FilePath& absolutePath) const
	{
		// Do nothing
		return true;
	}

	bool MeshSource::Deserialize(const FilePath& absolutePath)
	{
		MeshSourceImporter importer(absolutePath);
		return importer.ImportToMeshSource(this);
	}

	StaticMesh::StaticMesh(AssetHandle meshSourceHandle)
	{
		m_MeshSource = meshSourceHandle;

		Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(meshSourceHandle);
		if (meshSource)
		{
			m_MaterialTable = meshSource->GetMaterialTable();
			m_SubMeshIndices = std::vector<uint32>(meshSource->GetSubMeshes().size());
			for (uint32 i = 0; i < m_SubMeshIndices.size(); ++i)
				m_SubMeshIndices[i] = i;
		}
	}

	bool StaticMesh::Serialize(const FilePath& absolutePath) const
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "StaticMesh" << YAML::Value << YAML::BeginMap;

		out << YAML::Key << "MeshSource" << YAML::Value << m_MeshSource;
		out << YAML::Key << "SubMeshIndices" << YAML::Value << m_SubMeshIndices;

		out << YAML::Key << "OverrideMaterials" << YAML::Value << YAML::BeginMap;
		for (const auto& [name, handle] : m_MaterialTable)
		{
			if (AssetManager::IsAssetHandleValid(handle) && !AssetManager::GetAssetMetadata(handle).IsMemoryOnly)
				out << YAML::Key << name << YAML::Value << handle;
		}
		out << YAML::EndMap;

		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream fout(absolutePath);
		fout << out.c_str();

		return true;
	}

	bool StaticMesh::Deserialize(const FilePath& absolutePath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(absolutePath.string());

			auto staticMeshNode = data["StaticMesh"];

			m_MeshSource = staticMeshNode["MeshSource"].as<AssetHandle>();
			m_SubMeshIndices = staticMeshNode["SubMeshIndices"].as<std::vector<uint32>>();

			Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(m_MeshSource);
			if (meshSource)
			{
				YAML::Node materialsNode = staticMeshNode["OverrideMaterials"];
				m_MaterialTable = meshSource->GetMaterialTable();
				for (auto& [name, handle] : m_MaterialTable)
				{
					YAML::Node materialNode = materialsNode[name];

					if (materialNode)
					{
						AssetHandle storedHandle = materialNode.as<AssetHandle>();
						if (AssetManager::IsAssetHandleValid(storedHandle))
							handle = storedHandle;
					}
				}
			}
		}
		catch (YAML::Exception& e)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to load static mesh asset data from {}. Error message:\n {}", absolutePath, e.what());
			return false;
		}

		return true;
	}

	SkeletalMesh::SkeletalMesh(AssetHandle meshSourceHandle)
	{
		m_MeshSource = meshSourceHandle;

		Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(meshSourceHandle);
		if (meshSource)
		{
			m_MaterialTable = meshSource->GetMaterialTable();
		}
	}

	bool SkeletalMesh::Serialize(const FilePath& absolutePath) const
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "SkeletalMesh" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "MeshSource" << YAML::Value << m_MeshSource;

		out << YAML::Key << "OverrideMaterials" << YAML::Value << YAML::BeginMap;
		for (const auto& [name, handle] : m_MaterialTable)
		{
			if (AssetManager::IsAssetHandleValid(handle) && !AssetManager::GetAssetMetadata(handle).IsMemoryOnly)
				out << YAML::Key << name << YAML::Value << handle;
		}
		out << YAML::EndMap;

		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream fout(absolutePath);
		fout << out.c_str();

		return true;
	}

	bool SkeletalMesh::Deserialize(const FilePath& absolutePath)
	{
		YAML::Node data;
		try
		{
			data = YAML::LoadFile(absolutePath.string());

			auto skeletalMeshNode = data["SkeletalMesh"];

			m_MeshSource = skeletalMeshNode["MeshSource"].as<AssetHandle>();

			Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(m_MeshSource);
			if (meshSource)
			{
				YAML::Node materialsNode = skeletalMeshNode["OverrideMaterials"];
				m_MaterialTable = meshSource->GetMaterialTable();
				for (auto& [name, handle] : m_MaterialTable)
				{
					YAML::Node materialNode = materialsNode[name];

					if (materialNode)
					{
						AssetHandle storedHandle = materialNode.as<AssetHandle>();
						if (AssetManager::IsAssetHandleValid(storedHandle))
							handle = storedHandle;
					}
				}
			}
		}
		catch (YAML::Exception& e)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to load skeletal mesh asset data from {}. Error message:\n {}", absolutePath, e.what());
			return false;
		}

		return true;
	}
}
