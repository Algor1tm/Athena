#pragma once

#include "Scene.h"


class aiScene;
class aiNode;

namespace Athena
{
	class ATHENA_API Importer3D
	{
	public:
		Importer3D(Ref<Scene> scene);
		~Importer3D();

		bool ImportScene(const Filepath& filepath);
		bool ImportStaticMesh(const Filepath& filepath, const StaticMeshImportInfo& info, Ref<StaticMesh> outMesh);

		void Release();

	private:
		void ProcessNode(const aiScene* aiscene, aiNode* node);
		const aiScene* OpenFile(const Filepath& filepath);

	private:
		Ref<Scene> m_Context;
		Filepath m_CurrentFilepath;
		std::unordered_map<Filepath, const aiScene*> m_ImportedScenes;
	};
}
