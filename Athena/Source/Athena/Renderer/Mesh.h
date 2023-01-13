#pragma once

#include "Athena/Core/Core.h"
#include "Buffer.h"

#include <vector>


namespace Athena
{
	struct SubMesh
	{
		String Name;
		Ref<VertexBuffer> Vertices;
	};

	class ATHENA_API StaticMesh
	{
	public:
		static Ref<StaticMesh> Create(const Filepath& filepath);

		const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
		const String& GetName() const { return m_Name; }
		const Filepath& GetFilepath() const { return m_Filepath; }

	private:
		std::vector<SubMesh> m_SubMeshes;
		String m_Name;
		Filepath m_Filepath;
	};
}
