#pragma once

#include "Athena/Core/Layer.h"
#include "Athena/Events/MouseEvent.h"
#include "Athena/Events/KeyEvent.h"
#include "Athena/Events/ApplicationEvent.h"


namespace Athena
{
	class ATHENA_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnAttach() override;
		void OnDetach() override;
		void OnEvent(Event& event) override;

		void Begin();
		void End();
		
		void BlockEvents(bool block) { m_BlockEvents = block; }

	private:
		bool m_BlockEvents = true;
		float m_Time = 0.f;
	};
}

