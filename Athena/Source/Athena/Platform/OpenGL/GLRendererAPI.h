#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/RendererAPI.h"


namespace Athena
{
	class ATHENA_API GLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;

		virtual void BindPipeline(const Pipeline& pipeline);

		virtual void BeginRenderPass(const RenderPass& pass);
		virtual void EndRenderPass();

		virtual void BeginComputePass(const ComputePass& pass);
		virtual void EndComputePass();

		virtual void DrawTriangles(const Ref<VertexBuffer>& vertexArray, uint32 indexCount = 0) override;
		virtual void DrawLines(const Ref<VertexBuffer>& vertexArray, uint32 vertexCount = 0) override;

		virtual void Dispatch(uint32 x, uint32 y, uint32 z, Vector3i workGroupSize) override;

	private:
		RenderPass m_CurrentRenderPass;
		ComputePass m_CurrentComputePass;
	};
}
