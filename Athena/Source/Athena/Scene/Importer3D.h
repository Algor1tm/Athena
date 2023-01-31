#pragma once

#include "Scene.h"


class aiScene;
class aiNode;


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

	private:
		Ref<Scene> m_Scene;
		Filepath m_SceneFilepath;
		std::unordered_map<Filepath, const aiScene*> m_ImportedScenes;
	};
}
