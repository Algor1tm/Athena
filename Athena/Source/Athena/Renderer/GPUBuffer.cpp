#include "GPUBuffer.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Athena/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"
#include "Athena/Platform/Vulkan/VulkanStorageBuffer.h"


namespace Athena
{
	Ref<IndexBuffer> IndexBuffer::Create(const IndexBufferCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanIndexBuffer>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(const VertexBufferCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanVertexBuffer>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}

	Ref<UniformBuffer> UniformBuffer::Create(const String& name, uint64 size)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanUniformBuffer>::Create(name, size);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}


	Ref<StorageBuffer> StorageBuffer::Create(const String& name, uint64 size, BufferMemoryFlags flags)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanStorageBuffer>::Create(name, size, flags);;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
