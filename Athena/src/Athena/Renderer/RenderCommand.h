#pragma once

#include "RendererAPI.h"


namespace Athena
{
	class RenderCommand
	{
	public:
		static inline void Init() 
		{
			s_RendererAPI->Init();
		}

		static inline void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		static inline void Clear(const Color& color) 
		{ 
			s_RendererAPI->Clear(color); 
		}

		static inline void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32 indexCount = 0)
		{
			s_RendererAPI->DrawIndexed(vertexArray, indexCount);
		}

	private:
		static RendererAPI* s_RendererAPI;
	};
}
