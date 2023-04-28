#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Framebuffer.h"


namespace Athena
{
	enum class CullFace
	{
		NONE, BACK, FRONT
	};

	enum class CullDirection
	{
		CLOCKWISE, COUNTER_CLOCKWISE
	};

	enum class DepthFunc
	{
		NONE, LESS, LEQUAL
	};

	enum class BlendFunc
	{
		NONE, ONE_MINUS_SRC_ALPHA, 
	};

	struct Pipeline
	{
		CullFace CullFace;
		CullDirection CullDirection;
		DepthFunc DepthFunc;
		BlendFunc BlendFunc;
	};

	enum ClearBit
	{
		CLEAR_NONE_BIT	  = BIT(0),
		CLEAR_COLOR_BIT   = BIT(1),
		CLEAR_DEPTH_BIT   = BIT(2),
		CLEAR_STENCIL_BIT = BIT(3)
	};

	struct RenderPass
	{
		Ref<Framebuffer> TargetFramebuffer;
		uint32 ClearBit = CLEAR_NONE_BIT;
		LinearColor ClearColor = LinearColor::Black;
		String Name;
	};

	enum BarrierBit
	{
		BARRIER_BIT_NONE = BIT(0),
		BUFFER_UPDATE_BARRIER_BIT = BIT(1),
		FRAMEBUFFER_BARRIER_BIT = BIT(2),
		SHADER_IMAGE_BARRIER_BIT = BIT(3),
		ALL_BARRIERS = BIT(4)
	};

	struct ComputePass
	{
		uint32 BarrierBit = ALL_BARRIERS;
		String Name;
	};

	class ATHENA_API RendererAPI
	{
	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;

		virtual void BindPipeline(const Pipeline& pipeline) = 0;

		virtual void BeginRenderPass(const RenderPass& pass) = 0;
		virtual void EndRenderPass() = 0;

		virtual void BeginComputePass(const ComputePass& pass) = 0;
		virtual void EndComputePass() = 0;

		virtual void DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount = 0) = 0;
		virtual void DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount = 0) = 0;

		virtual void Dispatch(uint32 x, uint32 y, uint32 z, Vector3i workGroupSize) = 0;
	};
}
