#pragma once

#include "Athena/Core/Core.h"

#include "Animation.h"
#include "GPUBuffers.h"
#include "AABB.h"

#include <vector>


class aiScene;
class aiNode;


namespace Athena
{
	struct SubMesh
	{
		String Name;
		Ref<VertexBuffer> VertexBuffer;
		String MaterialName;
	};

	class ATHENA_API StaticMesh
	{
	public:
		static Ref<StaticMesh> Create(const Filepath& path);

		const std::vector<SubMesh>& GetAllSubMeshes() const { return m_SubMeshes; }

		const String& GetName() const { return m_Name; }
		const Filepath& GetFilepath() const { return m_Filepath; }
		const AABB& GetBoundingBox() const { return m_AABB; }

		const Ref<Animator>& GetAnimator() { return m_Animator; }

		bool HasAnimations() const { return m_Animator != nullptr; }

	private:
		void ProcessNode(const aiScene* aiscene, const aiNode* ainode, const Matrix4& parentTransform);

	private:
		Filepath m_Filepath;
		String m_Name;
		AABB m_AABB;
		std::vector<SubMesh> m_SubMeshes;

		Ref<Skeleton> m_Skeleton;
		Ref<Animator> m_Animator;
	};
}
