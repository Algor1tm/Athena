#include "atnpch.h"
#include "RenderCommand.h"

#include "Athena/Platform/OpenGL/OpenGLRendererAPI.h"


namespace Athena
{
	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI();
}
