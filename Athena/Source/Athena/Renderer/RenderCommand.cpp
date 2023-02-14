#include "RenderCommand.h"

#include "Athena/Platform/OpenGL/GLRendererAPI.h"

#include "Renderer.h"

namespace Athena
{
	Scope<RendererAPI> RenderCommand::s_RendererAPI;

	void RenderCommand::Init()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			s_RendererAPI = CreateScope<GLRendererAPI>(); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		s_RendererAPI->Init();
	}
}
