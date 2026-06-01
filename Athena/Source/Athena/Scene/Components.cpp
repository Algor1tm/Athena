#include "Components.h"


namespace Athena
{
	void MeshComponent::ResetMaterials()
	{
		Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(MeshHandle);

		if (mesh)
		{
			const MaterialTable& meshMaterialTable = mesh->GetMaterialTable();

			if (mesh->IsCollapsedGraph())
			{
				OverrideMaterials = meshMaterialTable;
			}
			else
			{
				OverrideMaterials.clear();

				const MeshNode& meshNode = mesh->GetMeshNode(MeshNodeIndex);
				for (uint32 i = 0; i < meshNode.SubMeshes.size(); ++i)
				{
					const SubMesh& subMesh = mesh->GetSubMesh(meshNode.SubMeshes[i]);
					OverrideMaterials[subMesh.MaterialName] = meshMaterialTable.at(subMesh.MaterialName);
				}
			}
		}
	}

	Ref<Material> MeshComponent::GetMaterial(const Ref<Mesh>& mesh, const String& materialName) const
	{
		Ref<MaterialAsset> asset;

		if (OverrideMaterials.contains(materialName))
		{
			asset = AssetManager::GetAsset<MaterialAsset>(OverrideMaterials.at(materialName));
		}
		else
		{
			asset = mesh->GetMaterial(materialName);
		}

		if (asset && asset->GetMaterial())
			return asset->GetMaterial();

		return MaterialAsset::GetDefault()->GetMaterial();
	}
}
