#pragma once

#include "Athena/Renderer/RendererAPI.h"


namespace Athena
{
	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		void Init() override;
		void Clear(const Color& color) override;
		void DrawIndexed(const Ref<VertexArray>& vertexArray) override;
	};
}