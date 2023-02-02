#pragma once

#include "Scene.h"


class aiScene;
class aiNode;
class aiMaterial;


namespace Athena
{
	struct SceneNode;

	class ATHENA_API Importer3D
	{
	public:
		Importer3D(const Ref<Scene>& scene);
		~Importer3D();

		bool Import(const Filepath& filepath);
		Ref<StaticMesh> ImportStaticMesh(const Filepath& filepath, uint32 aiMeshIndex);

		void Release();

	private:
		const aiScene* OpenFile(const Filepath& filepath);
		void ProcessNode(const aiScene* aiscene, const SceneNode* node);

		Ref<Texture2D> LoadTexture(const aiScene* scene, const aiMaterial* aimaterial, uint32 type);
		Ref<Material> LoadMaterial(const aiScene* scene, uint32 aiMaterialIndex);
		Ref<StaticMesh> LoadStaticMesh(const aiScene* scene, uint32 aiMeshIndex);

	private:
		Ref<Scene> m_Scene;
		Filepath m_SceneFilepath;
		std::unordered_map<Filepath, const aiScene*> m_ImportedScenes;
	};
}
