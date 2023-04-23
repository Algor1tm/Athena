#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Framebuffer.h"


namespace Athena
{
	enum class CullFace
	{
		BACK, FRONT
	};

	enum class CullDirection
	{
		CLOCKWISE, COUNTER_CLOCKWISE
	};

	class ATHENA_API RendererAPI
	{
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height) = 0;
		virtual void Clear(const LinearColor& color) = 0;

		virtual void DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount = 0) = 0;
		virtual void DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount = 0) = 0;

		virtual void Dispatch(uint32 x, uint32 y, uint32 z, Vector3i workGroupSize) = 0;

		virtual void DisableCulling() = 0;
		virtual void SetCullMode(CullFace face = CullFace::BACK, CullDirection direction = CullDirection::COUNTER_CLOCKWISE) = 0;
	};
}
