#pragma once 

#include "Athena/Core/Core.h"
#include "Athena/Math/Matrix.h"
#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Pipeline.h"

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
		DrawList(bool isAnimated = false);

		void Push(const DrawCall& drawCall);
		void Sort();

		void Flush(const Ref<Pipeline>& pipeline);
		void FlushShadowPass(const Ref<Pipeline>& pipeline);

		void Clear();

	private:
		bool UpdateMaterial(const DrawCall& drawCall);

	private:
		std::vector<DrawCall> m_Array;
		Ref<Material> m_LastMaterial = nullptr;
		bool m_IsAnimated;
	};
}
