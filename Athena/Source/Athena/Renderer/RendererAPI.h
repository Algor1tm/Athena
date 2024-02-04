#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/RenderCommandBuffer.h"


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

		virtual void RenderGeometry(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<VertexBuffer>& mesh, const Ref<Material>& material) = 0;

		virtual void BlitToScreen(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Texture2D>& texture) = 0;
		virtual void WaitDeviceIdle() = 0;

		virtual void GetRenderCapabilities(RenderCapabilities& caps) = 0;
		virtual uint64 GetMemoryUsage() = 0;
	};
}
