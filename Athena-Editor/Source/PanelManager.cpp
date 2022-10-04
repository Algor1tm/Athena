#include "PanelManager.h"

#include "Athena/Core/Log.h"
#include "Athena/Input/Input.h"

#include <ImGui/imgui.h>


namespace Athena
{
	static const char* KeyToString(Keyboard::Key key)
	{
		switch (key)
		{
		case Keyboard::Space: return "Ctrl+Space";
		}

		return NULL;
	}

	void PanelManager::OnImGuiRender()
	{
		for (auto& [key, panel] : m_Panels)
		{
			if (panel.IsOpen)
			{
				panel.PanelRef->OnImGuiRender();
			}
		}
	}

	void PanelManager::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyPressedEvent>(ATN_BIND_EVENT_FN(PanelManager::OnKeyPressedEvent));
	}

	void PanelManager::ImGuiRenderAsMenuItems()
	{
		for (auto& [key, panel] : m_Panels)
		{
			if (panel.IsHideable)
			{
				if (ImGui::MenuItem(panel.PanelRef->GetName().data(), KeyToString(panel.HotKey), panel.IsOpen))
				{
					panel.IsOpen = !panel.IsOpen;
				}
			}
		}
	}

	void PanelManager::AddPanel(const Ref<Panel>& panel, bool isHideable)
	{
		PanelDescription desc;
		desc.PanelRef = panel;
		desc.IsOpen = true;
		desc.IsHideable = isHideable;
		m_Panels[panel->GetName()] = desc;
	}

	void PanelManager::AddPanel(const Ref<Panel>& panel, Keyboard::Key hotkey)
	{
		PanelDescription desc;
		desc.PanelRef = panel;
		desc.IsOpen = true;
		desc.IsHideable = true;
		desc.HotKey = hotkey;
		m_Panels[panel->GetName()] = desc;
	}

	bool PanelManager::IsPanelOpen(std::string_view name)
	{
		return m_Panels.at(name).IsOpen;
	}

	void PanelManager::RenderPanel(std::string_view name, bool enable)
	{
		m_Panels.at(name).IsOpen = enable;
	}

	Ref<Panel> PanelManager::GetPanel(std::string_view name)
	{
		return m_Panels.at(name).PanelRef;
	}

	bool PanelManager::OnKeyPressedEvent(KeyPressedEvent& event)
	{
		bool ctrl = Input::IsKeyPressed(Keyboard::LCtrl) || Input::IsKeyPressed(Keyboard::RCtrl);

		if (ctrl)
		{
			auto keycode = event.GetKeyCode();
			for (auto& [key, panel] : m_Panels)
			{
				if (panel.HotKey != Keyboard::Escape && panel.HotKey == keycode)
				{
					panel.IsOpen = !panel.IsOpen;
				}
			}
		}

		return false;
	}
}
