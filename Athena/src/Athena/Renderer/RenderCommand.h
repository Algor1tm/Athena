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

		static inline void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		static inline void Clear(const Color& color) 
		{ 
			s_RendererAPI->Clear(color); 
		}

		static inline void DrawIndexed(const Ref<VertexArray>& vertexArray)
		{
			s_RendererAPI->DrawIndexed(vertexArray);
		}

	private:
		static RendererAPI* s_RendererAPI;
	};
}
