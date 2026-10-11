#pragma once 

#include "Athena/Core/Core.h"
#include "Athena/Renderer/RHI/RHI.h"


namespace Athena
{
	class ATHENA_API VulkanRHI : public RHI
	{
	public:
		virtual void Init(const RendererConfig& config) override;
		virtual void Shutdown() override;

	private:

	};
}
