#include "Mesh.h"


namespace Athena
{
	Ref<SkeletalMesh> SkeletalMesh::Create(const SkeletalMeshDescription& desc)
	{
		Ref<SkeletalMesh> result = CreateRef<SkeletalMesh>();
		result->m_SubMeshes = desc.SubMeshes;
		result->m_Name = desc.Name;
		result->m_Filepath = desc.Filepath;

		result->m_Skeleton = desc.Skeleton;
		result->m_Animations = desc.Animations;

		return result;
	}

	bool SkeletalMesh::HasAnimation(const Ref<Animation>& animation)
	{
		for (const auto& anim : m_Animations)
		{
			if (anim == animation)
				return true;
		}

		return false;
	}
}
