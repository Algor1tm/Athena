#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RenderThread.h"

namespace Athena
{
	class ATHENA_API RHI
	{
	public:
		static RHI* Get() { return s_RHI; }

		virtual ~RHI() = default;

		virtual void Init(const RendererConfig& config) = 0;
		virtual void Shutdown() = 0;

		friend class RenderThread;

	private:
		// Initialized and destroyed in RenderThread
		static inline RHI* s_RHI = nullptr;
	};
}