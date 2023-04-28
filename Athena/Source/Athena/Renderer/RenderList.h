#pragma once 

#include "Athena/Core/Core.h"

#include "Athena/Math/Matrix.h"

#include <deque>


namespace Athena
{
	class ATHENA_API Animator;
	class ATHENA_API VertexBuffer;
	class ATHENA_API Material;

	struct DrawCallInfo
	{
		Ref<VertexBuffer> VertexBuffer;
		Ref<Material> Material;
		Ref<Animator> Animator;
		Matrix4 Transform;
		int32 EntityID;
	};


	class ATHENA_API RenderList
	{
	public:
		RenderList() = default;

		void Push(const DrawCallInfo& info);
		const DrawCallInfo& Next();

		void Reset() { m_CurrentIndex = 0; m_LastMaterial = nullptr; m_LastAnimator = nullptr; }

		void Clear();

		uint32 Size() const { return m_Queue.size(); }
		bool Empty() const { return m_Queue.empty(); }

		void Sort();

		bool HasStaticMeshes() const;
		bool HasAnimMeshes() const;

		bool UpdateMaterial();
		bool UpdateAnimator();

	private:
		bool CheckLimit() const;

	private:
		std::deque<DrawCallInfo> m_Queue;
		uint32 m_CurrentIndex = 0;

		Ref<Material> m_LastMaterial;
		Ref<Animator> m_LastAnimator;
	};
}
