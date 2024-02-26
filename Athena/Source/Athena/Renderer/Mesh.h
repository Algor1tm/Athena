#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/AABB.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Material.h"

#include <vector>


class aiScene;
class aiNode;


namespace Athena
{
	struct SubMesh
	{
		String Name;
		Ref<VertexBuffer> VertexBuffer;
		Ref<Material> Material;
	};

	class ATHENA_API StaticMesh : public RefCounted
	{
	public:
		static Ref<StaticMesh> Create(const FilePath& path);

		const std::vector<SubMesh>& GetAllSubMeshes() const { return m_SubMeshes; }

		const String& GetName() const { return m_Name; }
		const FilePath& GetFilePath() const { return m_FilePath; }
		const AABB& GetBoundingBox() const { return m_AABB; }

		const Ref<Animator>& GetAnimator() { return m_Animator; }

		bool HasAnimations() const { return m_Animator != nullptr; }

	private:
		void ProcessNode(const aiScene* aiscene, const aiNode* ainode, const Matrix4& parentTransform);

	private:
		FilePath m_FilePath;
		String m_Name;
		AABB m_AABB;
		std::vector<SubMesh> m_SubMeshes;

		Ref<Skeleton> m_Skeleton;
		Ref<Animator> m_Animator;
	};
}
