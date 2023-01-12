#include "Mesh.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>


namespace Athena
{
	Ref<Mesh> Mesh::Create(const Ref<VertexBuffer>& vertexBuffer)
	{
		auto mesh = CreateRef<Mesh>();
		mesh->m_VertexBuffer = vertexBuffer;
		return mesh;
	}

	Ref<Mesh> Mesh::LoadFromFile(const Filepath& filepath)
	{
		const aiScene* scene = aiImportFile(filepath.string().c_str(), 0);

		auto mesh = CreateRef<Mesh>();
		return mesh;
	}

	Ref<VertexBuffer> Mesh::GetVertexBuffer()
	{
		return m_VertexBuffer;
	}
}
