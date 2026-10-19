#include "Mesh.h"

#include "Athena/Asset/AssetManager.h"
#include "Athena/Asset/Editor/MeshImporter.h"
#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/EngineRenderAssets.h"


namespace Athena
{
	Mesh::Mesh()
	{

	}

	Mesh::~Mesh()
	{
		ENQUEUE_RENDER_COMMAND([vertexBuffer = m_VertexBuffer, indexBuffer = m_IndexBuffer, bonesInfluenceBuffer = m_BonesInfluenceBuffer]() mutable
			{
				vertexBuffer.Release();
				indexBuffer.Release();
				bonesInfluenceBuffer.Release();
			});
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
			return EngineRenderAssets::Get().DefaultMaterialAsset;
		}

		Ref<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(materialHandle);
		materialAsset = materialAsset ? materialAsset : EngineRenderAssets::Get().DefaultMaterialAsset;

		return materialAsset;
	}
}
