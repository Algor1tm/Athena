#include "atnpch.h"
#include "RenderCommand.h"
#include "Renderer.h"

#include "Athena/Platform/Direct3D/D3D11RendererAPI.h"
#include "Athena/Platform/OpenGL/GLRendererAPI.h"


namespace Athena
{
	Scope<RendererAPI> RenderCommand::s_RendererAPI;

	void RenderCommand::Init()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			s_RendererAPI = CreateScope<GLRendererAPI>(); break;
		case RendererAPI::API::Direct3D:
			s_RendererAPI = CreateScope<D3D11RendererAPI>(); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		s_RendererAPI->Init();
	}
}
