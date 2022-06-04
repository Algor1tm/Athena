#pragma once

#include "RendererAPI.h"


namespace Athena
{
	class RenderCommand
	{
	public:
		static inline void Clear(const Color& color) 
		{ 
			s_RendererAPI->Clear(color); 
		}

		static inline void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray)
		{
			s_RendererAPI->DrawIndexed(vertexArray);
		}

	private:
		static RendererAPI* s_RendererAPI;
	};
}
