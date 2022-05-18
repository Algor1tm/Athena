#pragma once

#include "Athena/Core.h"
#include "Athena/Layer.h"


namespace Athena
{
	class ATHENA_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate() override;
		void OnEvent(Event& event) override;
	private:
		float m_Time = 0.f;
	};
}

