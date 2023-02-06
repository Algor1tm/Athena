#pragma once

#include "Athena/Core/Core.h"

#include "Animation.h"
#include "GPUBuffers.h"
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



	struct SubMesh
	{
		String Name = "UnNamed";
		Ref<VertexBuffer> VertexBuffer;
		String MaterialName;
		Matrix4 LocalTransform = Matrix4::Identity();
	};

	struct SkeletalMeshDescription
	{
		std::vector<SubMesh> SubMeshes;
		String Name;
		Filepath Filepath;

		Ref<Skeleton> Skeleton = nullptr;
		std::vector<Ref<Animation>> Animations;
	};

	class ATHENA_API SkeletalMesh
	{
	public:
		static Ref<SkeletalMesh> Create(const SkeletalMeshDescription& desc);

		uint32 GetSubMeshesCount() const { return m_SubMeshes.size(); }
		const SubMesh& GetSubMesh(uint32 index) { return m_SubMeshes[index]; }

		const String& GetName() const { return m_Name; }
		const Filepath GetFilepath() const { return m_Filepath; }

		bool HasAnimation(const Ref<Animation>& animation);
		const std::vector<Ref<Animation>>& GetAllAnimations() const { return m_Animations; };

	private:
		std::vector<SubMesh> m_SubMeshes;
		String m_Name;
		Filepath m_Filepath;

		Ref<Skeleton> m_Skeleton;
		std::vector<Ref<Animation>> m_Animations;
	};
}
