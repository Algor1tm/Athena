#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Input/Events/Event.h"
#include "Athena/Input/Events/KeyEvent.h"
#include "Athena/Input/Keyboard.h"

#include "Panels/Panel.h"

#include <vector>


namespace Athena
{
	struct PanelDescription
	{
		Ref<Panel> PanelRef;
		bool IsOpen = true;
		Keyboard::Key HotKey = Keyboard::Escape;
	};


	class PanelManager
	{
	public:
		void OnImGuiRender();
		void OnEvent(Event& event);

		void AddPanel(const Ref<Panel>& panel, Keyboard::Key hotkey = Keyboard::Escape);

		bool IsPanelOpen(std::string_view name);
		void RenderPanel(std::string_view name, bool enable);
		Ref<Panel> GetPanel(std::string_view name);

	private:
		PanelDescription* GetPanelDesc(std::string_view name);
		bool OnKeyPressedEvent(KeyPressedEvent& event);

	private:
		std::vector<PanelDescription> m_Panels;
	};
}
