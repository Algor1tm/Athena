#pragma once

#include "Athena/Core/Core.h"
#include "Buffer.h"


namespace Athena
{
	class ATHENA_API Mesh
	{
	public:
		Mesh(const Ref<VertexBuffer>& vertexBuffer) {}
		Mesh(const Filepath& filepath) {}

	private:
		Ref<VertexBuffer> m_VertexBuffer;
	};
}
