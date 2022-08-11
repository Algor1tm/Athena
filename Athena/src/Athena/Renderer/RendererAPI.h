#pragma once

#include "Athena/Core/Color.h"
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
		virtual void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height) = 0;
		virtual void Clear(const LinearColor& color) = 0;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32 indexCount = 0) = 0;

		static inline API GetAPI() { return s_API; }
	private:
		static API s_API;
	};
}
