#pragma once 

#include "Athena/Core/Core.h"

#include "Athena/Math/Matrix.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Material.h"

#include <deque>


namespace Athena
{
	struct DrawCall
	{
		Ref<VertexBuffer> VertexBuffer;
		Ref<Material> Material;
		Matrix4 Transform;
		uint32 BonesOffset = 0;
	};


	class ATHENA_API DrawList
	{
	public:
		DrawList() = default;

		void Push(const DrawCall& drawCall);
		void Sort();

		uint32 Size() const { return m_Array.size(); }
		bool Empty() const { return m_Array.empty(); }
		void Clear();

		bool UpdateMaterial(const DrawCall& drawCall);

		std::vector<DrawCall>::const_iterator begin();
		std::vector<DrawCall>::const_iterator end();

	private:
		std::vector<DrawCall> m_Array;
		Ref<Material> m_LastMaterial = nullptr;
	};
}
