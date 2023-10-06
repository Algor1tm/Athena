#include "GPUBuffers.h"

#include "Athena/Renderer/Renderer.h"


namespace Athena
{
	Ref<IndexBuffer> IndexBuffer::Create(uint32* vertices, uint32 count)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::None: return nullptr;
		case Renderer::API::Vulkan: return nullptr;
		}

		return nullptr;
	}


	Ref<VertexBuffer> VertexBuffer::Create(const VertexBufferCreateInfo& info)
	{
		ATN_CORE_ASSERT(!(info.Usage == BufferUsage::STATIC && info.Data == nullptr), "Invalid vertex buffer data!");

		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return nullptr;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}


	Ref<ConstantBuffer> ConstantBuffer::Create(uint32 size, uint32 binding)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return nullptr;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}


	Ref<ShaderStorageBuffer> ShaderStorageBuffer::Create(uint32 size, uint32 binding)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return nullptr;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
