#include "GPUBuffers.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Athena/Platform/Vulkan/VulkanUniformBuffer.h"


namespace Athena
{
	Ref<VertexBuffer> VertexBuffer::Create(const VertexBufferCreateInfo& info)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanVertexBuffer>::Create(info);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}


	Ref<UniformBuffer> UniformBuffer::Create(uint64 size)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return Ref<VulkanUniformBuffer>::Create(size);
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}


	Ref<ShaderStorageBuffer> ShaderStorageBuffer::Create(uint64 size)
	{
		switch (Renderer::GetAPI())
		{
		case Renderer::API::Vulkan: return nullptr;
		case Renderer::API::None: return nullptr;
		}

		return nullptr;
	}
}
