#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/RenderCommandBuffer.h"
#include "Athena/Renderer/Pipeline.h"


namespace Athena
{
	class ATHENA_API RendererAPI : public RefCounted
	{
	public:
		static Ref<RendererAPI> Create(Renderer::API api);
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void OnUpdate() = 0;

		virtual void RenderGeometry(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Pipeline>& pipeline, const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, uint32 vertexCount = 0) = 0;
		virtual void Dispatch(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<ComputePipeline>& pipeline, Vector3i workGroupSize, const Ref<Material>& material) = 0;
		virtual void MemoryDependency(const Ref<RenderCommandBuffer>& cmdBuffer) = 0;
		virtual void ExecutionDependency(const Ref<RenderCommandBuffer>& cmdBuffer) = 0;

		virtual void BlitToScreen(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Image>& image) = 0;
		virtual void WaitDeviceIdle() = 0;

		virtual void BeginDebugRegion(const Ref<RenderCommandBuffer>& cmdBuffer, std::string_view name, const Vector4& color) = 0;
		virtual void EndDebugRegion(const Ref<RenderCommandBuffer>& cmdBuffer) = 0;
		virtual void InsertDebugMarker(const Ref<RenderCommandBuffer>& cmdBuffer, std::string_view name, const Vector4& color) = 0;

		virtual void GetRenderCapabilities(RenderCapabilities& caps) = 0;
		virtual uint64 GetMemoryUsage() = 0;
	};
}
