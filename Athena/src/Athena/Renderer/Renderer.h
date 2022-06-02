#pragma once


namespace Athena
{
	enum class RendererAPI
	{
		None = 0, OpenGL, Direct3D
	};


	class ATHENA_API Renderer
	{
	public:
		static inline RendererAPI GetAPI() { return s_RendererAPI; }

	private:
		static RendererAPI s_RendererAPI;
	};
}
