#pragma once

#include "Athena/Core/Core.h"

#include "Athena/ImGui/ImGuiLayer.h"


namespace Athena
{
	class ATHENA_API D3D11ImGuiLayerImpl : public ImGuiLayerImpl
	{
	public:
		virtual void Init(void* windowHandle) override;
		virtual void Shutdown() override;

		virtual void NewFrame() override;
		virtual void RenderDrawData() override;

		virtual void UpdateViewports() override;
	};
}
