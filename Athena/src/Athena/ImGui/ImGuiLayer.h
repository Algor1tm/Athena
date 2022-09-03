#pragma once

#include "Athena/Core/Layer.h"
#include "Athena/Input/Events/MouseEvent.h"
#include "Athena/Input/Events/KeyEvent.h"
#include "Athena/Input/Events/ApplicationEvent.h"


namespace Athena
{
	class ATHENA_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void SetDarkTheme();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& event) override;

		void Begin();
		void End();
		
		void BlockEvents(bool block) { m_BlockEvents = block; }

	private:
		bool m_BlockEvents = true;
		float m_Time = 0.f;
	};
}

