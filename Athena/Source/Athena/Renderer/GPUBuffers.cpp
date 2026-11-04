#include "GPUBuffers.h"
#include "Athena/Renderer/RenderThread.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	void VertexMemoryLayout::CalculateOffsetsAndStride()
	{
		uint32 offset = 0;
		for (auto& elem : m_Elements)
		{
			ensure(GetFormatSupportProperties(elem.ElementFormat).IsVertexBufferSupported, "Format is not supported for vertex buffers!");
			uint32 elemSize = GetFormatProperties(elem.ElementFormat).BytesPerPixel;

			elem.Offset = offset;
			offset += elemSize;
			m_Stride += elemSize;
		}
	}

	Ref<GPUBuffer> GPUBuffer::Create(const GPUBufferCreateInfo& info, Buffer* data)
	{
		TRACY_PROFILE_FUNC();

		Ref<GPUBuffer> buffer = RHI::Get()->CreateGPUBuffer(info);

		if (data)
		{
			Buffer dataCopy = Buffer::Copy(*data);
			check(buffer->GetSize() >= dataCopy.Size() && dataCopy.Data() != nullptr, "Invalid initialization buffer for VertexBuffer!");

			ENQUEUE_RENDER_COMMAND([buffer, dataCopyRT = std::move(dataCopy)]()
				{
					buffer->Init();

					Ref<CommandContext> cmdContext = RHI::Get()->GetContext(CommandContextType::AsyncTransfer);

					cmdContext->Transition(buffer.Raw(), PA_Unknown, PA_CopyDst);
					cmdContext->UploadBuffer(buffer.Raw(), dataCopyRT, 0);
					cmdContext->TransitionOwnership(buffer.Raw(), PA_CopyDst, CommandContextType::Graphics, PA_VertexInput);
				});
		}
		else
		{
			ENQUEUE_RENDER_COMMAND([buffer]()
				{
					buffer->Init();
				});
		}

		return buffer;
	}

	GPUBuffer::GPUBuffer(const GPUBufferCreateInfo& info)
		: m_Info(info)
	{

	}
}
