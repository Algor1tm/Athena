#pragma once

#include "Athena/Asset/Asset.h"
#include "Athena/Core/Core.h"
#include "Athena/Renderer/Mesh.h"
#include "Athena/Renderer/Animation.h"


class aiScene;
class aiNode;
class aiMesh;
class aiMaterial;


namespace Athena
{
	class AssimpImporter
	{
	public:
		AssimpImporter(const FilePath& path);
		~AssimpImporter();

		Ref<MeshSource> ImportToMeshSource() const;

	private:
		void TraverseNodes(const Ref<MeshSource>& meshSource, const aiNode* ainode, const Matrix4& parentTransform = Matrix4::Identity()) const;

		SubMesh LoadSubMesh(const Ref<MeshSource>& meshSource, const aiMesh* aimesh, const Matrix4& transform) const;
		Ref<VertexBuffer> LoadStaticVertexBuffer(const aiMesh* aimesh, const Matrix4& transform) const;
		AssetHandle LoadMaterial(const aiMaterial* aimaterial) const;
		AssetHandle LoadMaterialTexture(const aiMaterial* aimaterial, uint32 type, bool srgb) const;

	private:
		FilePath m_Path;
		const aiScene* m_aiScene;
	};
}
