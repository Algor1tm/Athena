#include "atnpch.h"
#include "VertexArray.h"
#include "Renderer.h"
#include "Athena/Platform/OpenGL/OpenGLVertexArray.h"


namespace Athena
{
	VertexArray* VertexArray::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return new OpenGLVertexArray(); break;
		case RendererAPI::API::Direct3D:
			ATN_CORE_ASSERT(false, "Renderer API Direct3D is not supported"); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown renderer API!");
		return nullptr;
	}
}
