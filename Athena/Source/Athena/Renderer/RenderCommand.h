#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/RendererAPI.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Framebuffer.h"


namespace Athena
{
	class ATHENA_API RenderCommand
	{
	public:
		static void Init();

		static inline void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		static inline void Clear(const LinearColor& color) 
		{ 
			s_RendererAPI->Clear(color); 
		}

		static inline void DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount = 0)
		{
			s_RendererAPI->DrawTriangles(vertexBuffer, indexCount);
		}

		static inline void DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount = 0)
		{
			s_RendererAPI->DrawLines(vertexBuffer, vertexCount);
		}

		static inline void BindFramebuffer(const Ref<Framebuffer>& framebuffer)
		{
			s_RendererAPI->BindFramebuffer(framebuffer);
		}

		static inline void UnBindFramebuffer()
		{
			s_RendererAPI->UnBindFramebuffer();
		}

	private:
		static Scope<RendererAPI> s_RendererAPI;
	};
}
