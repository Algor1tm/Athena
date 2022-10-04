#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Input/Events/Event.h"
#include "Athena/Input/Events/KeyEvent.h"
#include "Athena/Input/Keyboard.h"

#include "Panels/Panel.h"

#include <unordered_map>


namespace Athena
{
	struct PanelDescription
	{
		Ref<Panel> PanelRef;
		bool IsOpen = true;
		bool IsHideable = true;
		Keyboard::Key HotKey = Keyboard::Escape;
	};


	class PanelManager
	{
	public:
		void OnImGuiRender();
		void OnEvent(Event& event);

		void ImGuiRenderAsMenuItems();

		void AddPanel(const Ref<Panel>& panel, bool isHideable = true);
		void AddPanel(const Ref<Panel>& panel, Keyboard::Key hotkey);

		bool IsPanelOpen(std::string_view name);
		void RenderPanel(std::string_view name, bool enable);
		Ref<Panel> GetPanel(std::string_view name);

	private:
		bool OnKeyPressedEvent(KeyPressedEvent& event);

	private:
		std::unordered_map<std::string_view, PanelDescription> m_Panels;
	};
}
