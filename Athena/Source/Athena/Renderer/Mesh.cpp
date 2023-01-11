#include "Mesh.h"


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
		auto mesh = CreateRef<Mesh>();
		return mesh;
	}

	Ref<VertexBuffer> Mesh::GetVertexBuffer()
	{
		return m_VertexBuffer;
	}
}