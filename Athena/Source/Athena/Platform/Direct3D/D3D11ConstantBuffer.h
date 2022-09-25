#pragma once

#include "Athena/Renderer/ConstantBuffer.h"
#include "D3D11GraphicsContext.h"


namespace Athena
{
	class ATHENA_API D3D11ConstantBuffer : public ConstantBuffer
	{
	public:
		D3D11ConstantBuffer(uint32 size, uint32 binding);

		virtual void SetData(const void* data, uint32 size, uint32 offset = 0) override;

	private:
		Microsoft::WRL::ComPtr<ID3D11Buffer> m_Buffer;
	};
}
