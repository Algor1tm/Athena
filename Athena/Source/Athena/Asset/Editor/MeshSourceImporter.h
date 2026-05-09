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
	class MeshSourceImporter
	{
	public:
		MeshSourceImporter(const FilePath& path);
		~MeshSourceImporter();

		bool ImportToMeshSource(WeakRef<MeshSource> meshSource) const;

		Ref<Animation> ImportAnimation(uint32 animationIndex, const Ref<Skeleton>& skeleton) const;
		Ref<Skeleton> ImportSkeleton() const;
		bool HasSkeleton() const;

	private:
		void TraverseNodes(const Ref<MeshSource>& meshSource, const aiNode* ainode, const Matrix4& parentTransform = Matrix4::Identity()) const;
		void BuildBonesHierarchy(const aiNode* ainode, const std::unordered_map<String, Matrix4>& bonesMap, const Matrix4& parentTransform, std::vector<Bone>& bones) const;

		void LoadGeometry(const Ref<MeshSource>& meshSource) const;
		AssetHandle LoadMaterial(const aiMaterial* aimaterial) const;
		AssetHandle LoadMaterialTexture(const aiMaterial* aimaterial, uint32 type, bool srgb) const;

	private:
		FilePath m_Path;
		const aiScene* m_aiScene;
	};
}
