#pragma once

#include "Athena/Renderer/RendererAPI.h"


namespace Athena
{
	class ATHENA_API OpenGLRendererAPI : public RendererAPI
	{
	public:
		void Init() override;
		void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height) override;
		void Clear(const LinearColor& color) override;
		void DrawTriangles(const Ref<VertexArray>& vertexArray, uint32 indexCount = 0) override;
		void DrawLines(const Ref<VertexArray>& vertexArray, uint32 vertexCount = 0) override;
		void SetLineWidth(float width) override;
	};
}
