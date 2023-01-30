#pragma once

#include "Athena/Core/Core.h"

#include "Buffer.h"
#include "AABB.h"

#include <vector>


namespace Athena
{
	struct StaticMesh
	{
		Ref<VertexBuffer> Vertices;
		AABB BoundingBox;
		String Name;
		String MaterialName;
		Filepath Filepath;
		uint32 aiMeshIndex;
	};
}
