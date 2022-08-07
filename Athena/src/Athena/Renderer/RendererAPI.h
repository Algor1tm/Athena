#pragma once

#include "Color.h"
#include "VertexArray.h"


namespace Athena
{
	class ATHENA_API RendererAPI
	{
	public:
		enum class API
		{
			None = 0, OpenGL = 1, Direct3D = 2
		};

	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void Clear(const Color& color) = 0;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;

		static inline API GetAPI() { return s_API; }
	private:
		static API s_API;
	};
}
