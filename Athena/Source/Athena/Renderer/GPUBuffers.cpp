#include "GPUBuffers.h"

#include "Athena/Platform/OpenGL/GLBuffers.h"

#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	Ref<IndexBuffer> IndexBuffer::Create(uint32* vertices, uint32 count)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::OpenGL:
			return CreateRef<GLIndexBuffer>(vertices, count); break;
		case Renderer::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}


	Ref<VertexBuffer> VertexBuffer::Create(const VertexBufferCreateInfo& info)
	{
		ATN_CORE_ASSERT(!(info.Usage == BufferUsage::STATIC && info.Data == nullptr), "Invalid vertex buffer data!");

		switch (Renderer::GetAPI())
		{
		case Renderer::API::OpenGL:
			return CreateRef<GLVertexBuffer>(info); break;
		case Renderer::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}


	Ref<ConstantBuffer> ConstantBuffer::Create(uint32 size, uint32 binding)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::OpenGL:
			return CreateRef<GLUniformBuffer>(size, binding); break;
		case Renderer::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}


	Ref<ShaderStorageBuffer> ShaderStorageBuffer::Create(uint32 size, uint32 binding)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::OpenGL:
			return CreateRef<GLShaderStorageBuffer>(size, binding); break;
		case Renderer::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
