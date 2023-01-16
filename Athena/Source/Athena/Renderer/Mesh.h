#pragma once

#include "Athena/Core/Core.h"

#include "Buffer.h"
#include "Material.h"

#include <vector>


namespace Athena
{
	class ATHENA_API Scene;

	class ATHENA_API StaticMesh
	{
	public:
		static Ref<StaticMesh> Create(const Filepath& filepath, const Ref<Scene>& scene);

		const std::vector<Ref<VertexBuffer>>& GetVertices() const { return m_Vertices; }

		const String& GetName() const { return m_Name; }
		const Filepath& GetFilepath() const { return m_Filepath; }
		SIZE_T GetMaterialIndex() const { return m_MaterialIndex; }

		void SetMaterialIndex(SIZE_T materialIndex) { m_MaterialIndex = materialIndex; }

	private:
		std::vector<Ref<VertexBuffer>> m_Vertices;
		SIZE_T m_MaterialIndex = 0;
		Filepath m_Filepath;
		String m_Name;
	};
}
