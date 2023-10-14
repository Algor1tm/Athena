#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/CommandBuffer.h"


namespace Athena
{
	enum class CullFace
	{
		NONE, BACK, FRONT
	};

	enum class CullDirection
	{
		CLOCKWISE, COUNTER_CLOCKWISE
	};

	enum class DepthFunc
	{
		NONE, LESS, LEQUAL
	};

	enum class BlendFunc
	{
		NONE, ONE_MINUS_SRC_ALPHA, 
	};


	class ATHENA_API RendererAPI
	{
	public:
		static Ref<RendererAPI> Create(Renderer::API api);
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void WaitDeviceIdle() = 0;
		virtual void Flush(const Ref<CommandBuffer>& cmdBuf) = 0;
	};
}
