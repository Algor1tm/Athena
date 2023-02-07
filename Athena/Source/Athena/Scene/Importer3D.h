#pragma once

#include "Scene.h"
#include "Athena/Renderer/Animation.h"


class aiScene;
class aiNode;
class aiMesh;
class aiMaterial;
class aiAnimation;


namespace Athena
{
	struct SceneNode;

	class ATHENA_API Importer3D
	{
	public:
		Importer3D(const Ref<Scene>& scene);
		~Importer3D();

		bool Import(const Filepath& path);
		Ref<StaticMesh> ImportStaticMesh(const Filepath& path, uint32 aiMeshIndex);
		Ref<SkeletalMesh> ImportSkeletalMesh(const Filepath& path);

		void Release();

	private:
		const aiScene* OpenFile(const Filepath& path);
		void ProcessNodeForStaticMeshes(const aiScene* aiscene, const SceneNode* node);
		void ProcessNodeForSubMeshes(const aiScene* aiscene, const SceneNode* node, std::vector<SubMesh>& subMeshes, const Ref<Skeleton>& skeleton);

		Ref<Texture2D> LoadTexture(const aiScene* aiscene, const aiMaterial* aimaterial, uint32 type);
		Ref<Material> LoadMaterial(const aiScene* aiscene, uint32 aiMaterialIndex);

		Ref<IndexBuffer> LoadIndexBuffer(const aiMesh* aimesh);
		Ref<VertexBuffer> LoadStaticVertexBuffer(const aiMesh* aimesh);
		Ref<VertexBuffer> LoadAnimVertexBuffer(const aiMesh* aimesh, const Ref<Skeleton>& skeleton);

		Ref<Animation> LoadAnimation(const aiAnimation* aianimation, const Ref<Skeleton>& skeleton);
		Ref<Skeleton> LoadSkeleton(const aiScene* aiscene);
		void LoadSkeletonRecursiveHelper(const aiNode* ainode, BoneStructureInfo& bone);

		Ref<SkeletalMesh> LoadSkeletalMesh(const aiScene* aiscene);
		Ref<StaticMesh> LoadStaticMesh(const aiScene* aiscene, uint32 aiMeshIndex);

	private:
		Ref<Scene> m_Scene;
		Filepath m_SceneFilepath;
		std::unordered_map<Filepath, const aiScene*> m_ImportedScenes;
	};
}
