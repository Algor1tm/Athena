#include "Mesh.h"

#include "Athena/Asset/AssetManager.h"
#include "Athena/Asset/Editor/MeshImporter.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Project/Project.h"
#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	Mesh::Mesh()
	{

	}

	bool Mesh::HasAnimation(const Ref<Animation>& animation) const
	{
		return std::find(m_Animations.begin(), m_Animations.end(), animation) != m_Animations.end();
	}

	bool Mesh::Serialize(const FilePath& absolutePath) const
	{
		// Do nothing
		return true;
	}

	bool Mesh::Deserialize(const FilePath& absolutePath, Ref<AssetImportSettings> importSettings)
	{
		m_CollapsedGraph = importSettings.As<MeshImportSettings>()->CollapseGraph;

		MeshImporter importer(importSettings);
		return importer.ImportToMesh(absolutePath, this);
	}

	Ref<MaterialAsset> Mesh::GetMaterial(const String& materialName) const
	{
		AssetHandle materialHandle = 0;
		if (m_MaterialTable.contains(materialName))
		{
			materialHandle = m_MaterialTable.at(materialName);
		}
		else
		{
			return MaterialAsset::GetDefault();
		}

		Ref<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(materialHandle);
		materialAsset = materialAsset ? materialAsset : MaterialAsset::GetDefault();

		return materialAsset;
	}
}
