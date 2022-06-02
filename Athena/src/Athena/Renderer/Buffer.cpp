#include "atnpch.h"
#include "Buffer.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/OpenGL/OpenGLBuffer.h"


namespace Athena
{
	VertexBuffer* VertexBuffer::Create(float* vertices, uint32_t count)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::OpenGL:
			return new OpenGLVertexBuffer(vertices, count); break;
		case RendererAPI::Direct3D:
			ATN_CORE_ASSERT(false, "Renderer API Direct3D is not supported"); break;
		case RendererAPI::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported"); 
		}

		return nullptr;
	}

	IndexBuffer* IndexBuffer::Create(uint32_t* vertices, uint32_t count)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::OpenGL:
			return new OpenGLIndexBuffer(vertices, count); break;
		case RendererAPI::Direct3D:
			ATN_CORE_ASSERT(false, "Renderer API Direct3D is not supported"); break;
		case RendererAPI::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		return nullptr;
	}
}
