#pragma once

#include "Athena/Core/Core.h"
#include "Buffer.h"


namespace Athena
{
	class ATHENA_API Mesh
	{
	public:
		static Ref<Mesh> Create(const Ref<VertexBuffer>& vertexBuffer);
		static Ref<Mesh> LoadFromFile(const Filepath& filepath);

		Ref<VertexBuffer> GetVertexBuffer();

	private:
		Ref<VertexBuffer> m_VertexBuffer;
	};
}
