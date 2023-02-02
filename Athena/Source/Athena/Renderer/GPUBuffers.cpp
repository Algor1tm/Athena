#include "GPUBuffers.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/OpenGL/GLBuffers.h"
#include "Athena/Platform/Direct3D/D3D11Buffers.h"


namespace Athena
{
	Ref<IndexBuffer> IndexBuffer::Create(uint32* vertices, uint32 count)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLIndexBuffer>(vertices, count); break;
		case RendererAPI::API::Direct3D:
			return CreateRef<D3D11IndexBuffer>(vertices, count); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}


	Ref<VertexBuffer> VertexBuffer::Create(const VertexBufferDescription& desc)
	{
		ATN_CORE_ASSERT(!(desc.Usage == BufferUsage::STATIC && desc.Data == nullptr), "Invalid vertex buffer data!");

		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLVertexBuffer>(desc); break;
		case RendererAPI::API::Direct3D:
			return CreateRef<D3D11VertexBuffer>(desc); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}


	Ref<ConstantBuffer> ConstantBuffer::Create(uint32 size, uint32 binding)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLUniformBuffer>(size, binding); break;
		case RendererAPI::API::Direct3D:
			return CreateRef<D3D11ConstantBuffer>(size, binding);
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}


	Ref<ShaderStorageBuffer> ShaderStorageBuffer::Create(uint32 size, uint32 binding)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<GLShaderStorageBuffer>(size, binding); break;
		case RendererAPI::API::None:
			ATN_CORE_ASSERT(false, "Renderer API None is not supported");
		}

		ATN_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
