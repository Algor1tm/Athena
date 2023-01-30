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

		bool Import(const Filepath& filepath);
		Ref<StaticMesh> ImportStaticMesh(const Filepath& filepath, uint32 aiMeshIndex);

		void Release();

	private:
		const aiScene* OpenFile(const Filepath& filepath);

	private:
		Ref<Scene> m_Scene;
		Filepath m_CurrentFilepath;
		std::unordered_map<Filepath, const aiScene*> m_ImportedScenes;
	};
}
