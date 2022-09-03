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

		static inline void Clear(const LinearColor& color) 
		{ 
			s_RendererAPI->Clear(color); 
		}

		static inline void DrawTriangles(const Ref<VertexArray>& vertexArray, uint32 indexCount = 0)
		{
			s_RendererAPI->DrawTriangles(vertexArray, indexCount);
		}

		static inline void DrawLines(const Ref<VertexArray>& vertexArray, uint32 vertexCount = 0)
		{
			s_RendererAPI->DrawLines(vertexArray, vertexCount);
		}

		static inline void SetLineWidth(float width)
		{
			s_RendererAPI->SetLineWidth(width);
		}

	private:
		static RendererAPI* s_RendererAPI;
	};
}
