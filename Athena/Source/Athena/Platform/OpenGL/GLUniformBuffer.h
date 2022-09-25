#pragma once

#include "Athena/Renderer/ConstantBuffer.h"


namespace Athena
{
	class ATHENA_API GLUniformBuffer: public ConstantBuffer
	{
	public:
		GLUniformBuffer(uint32 size, uint32 binding);
		virtual ~GLUniformBuffer();

		virtual void SetData(const void* data, uint32 size, uint32 offset = 0) override;

	private:
		uint32 m_RendererID = 0;
	};
}
