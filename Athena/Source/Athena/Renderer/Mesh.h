#pragma once

#include "Athena/Core/Core.h"

#include "Buffer.h"
#include "AABB.h"

#include <vector>


namespace Athena
{
	struct StaticMeshImportInfo
	{
		String Name;
		std::vector<uint32> Indices;
		uint32 MaterialIndex;
	};

	struct StaticMesh
	{
		std::vector<Ref<VertexBuffer>> Vertices;
		AABB BoundingBox;
		String MaterialName;
		Filepath Filepath;
		StaticMeshImportInfo ImportInfo;
	};
}
