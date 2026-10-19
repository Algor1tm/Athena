#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/RHI/Vulkan/VulkanRHI.h"


namespace Athena
{
	class ATHENA_API VulkanVertexBuffer : public VertexBuffer
	{
	public:
		VulkanVertexBuffer(const String& name, uint64 size);
		virtual ~VulkanVertexBuffer();

		virtual void Init() override;
		virtual void Upload(const Buffer& buffer, uint64 offset) override;
		virtual void Resize(uint64 size) override;

	private:
		void CleanUp();

	private:
		VulkanBufferAllocation m_VertexBuffer;
	};

	class ATHENA_API VulkanIndexBuffer : public IndexBuffer
	{
	public:
		VulkanIndexBuffer(const String& name, uint64 size, IndexSize indexSize);
		virtual ~VulkanIndexBuffer();

		virtual void Init() override;
		virtual void Upload(const Buffer& buffer, uint64 offset) override;
		virtual void Resize(uint64 size) override;

	private:
		void CleanUp();

	private:
		VulkanBufferAllocation m_IndexBuffer;
	};
}
