#include "atnpch.h"
#include "ConstantBuffer.h"

#include "Renderer.h"
#include "Athena/Platform/OpenGL/GLUniformBuffer.h"
#include "Athena/Platform/Direct3D/D3D11ConstantBuffer.h"


namespace Athena
{
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
}
