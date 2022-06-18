#pragma once

#include "Color.h"
#include "VertexArray.h"


namespace Athena
{
	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0, OpenGL = 1, Direct3D = 2
		};

	public:
		virtual void Init() = 0;
		virtual void Clear(const Color& color) = 0;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray) = 0;

		static inline API GetAPI() { return s_API; }
	private:
		static API s_API;
	};
}
