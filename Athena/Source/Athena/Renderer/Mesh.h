#pragma once

#include "Athena/Core/Core.h"
#include "Buffer.h"

#include <vector>


class aiMesh;
class aiScene;
class aiNode;

namespace Athena
{
	struct MeshNode
	{
		String Name;
		std::vector<Ref<VertexBuffer>> SubMeshes;
	};

	class ATHENA_API Mesh
	{
	public:
		static Ref<Mesh> Create(const Filepath& filepath);

		const std::vector<MeshNode>& GetNodes() const { return m_Nodes; }
		const String& GetName() const { return m_Name; }
		const Filepath& GetFilepath() const { return m_Filepath; }

	private:
		static Ref<VertexBuffer> AssimpMeshToVertexBuffer(aiMesh* aimesh);
		static void AssimpNodesToAthenaNodes(const aiScene* scene, aiNode* node, std::vector<MeshNode>& storage);

	private:
		std::vector<MeshNode> m_Nodes;
		String m_Name;
		Filepath m_Filepath;
	};
}
