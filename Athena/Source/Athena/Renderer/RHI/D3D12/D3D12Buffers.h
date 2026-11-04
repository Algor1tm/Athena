#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RHI/D3D12/D3D12RHI.h"


namespace Athena
{
	class ATHENA_API D3D12GPUBuffer : public GPUBuffer
	{
	public:
		D3D12GPUBuffer(const GPUBufferCreateInfo& info);
		virtual ~D3D12GPUBuffer();

		virtual void Init() override;

	private:
		D3D12Resource m_Buffer;
	};
}
