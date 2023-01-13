#pragma once

#include "Color.h"
#include "Buffer.h"
#include "Framebuffer.h"


namespace Athena
{
	class ATHENA_API RendererAPI
	{
	public:
		enum API
		{
			None = 0, OpenGL = 1, Direct3D = 2
		};

	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height) = 0;
		virtual void Clear(const LinearColor& color) = 0;

		virtual void DrawTriangles(const Ref<VertexBuffer>& vertexBuffer, uint32 indexCount = 0) = 0;
		virtual void DrawLines(const Ref<VertexBuffer>& vertexBuffer, uint32 vertexCount = 0) = 0;

		virtual void BindFramebuffer(const Ref<Framebuffer>& framebuffer) = 0;
		virtual void UnBindFramebuffer() = 0;

		static inline API GetAPI() { return s_API; }
		static inline void Init(RendererAPI::API api) { s_API = api; }

	private:
		static inline API s_API = API::None;
	};
}
