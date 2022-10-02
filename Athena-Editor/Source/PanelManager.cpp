#include "PanelManager.h"

#include "Athena/Core/Log.h"
#include "Athena/Input/Input.h"


namespace Athena
{
	void PanelManager::OnImGuiRender()
	{
		for (auto& panel : m_Panels)
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

	void PanelManager::AddPanel(const Ref<Panel>& panel, Keyboard::Key hotkey)
	{
		m_Panels.push_back({ panel, true, hotkey });
	}

	bool PanelManager::IsPanelOpen(std::string_view name)
	{
		auto* panel = GetPanelDesc(name);

		if (panel != nullptr)
			return panel->IsOpen;

		return false;
	}

	void PanelManager::RenderPanel(std::string_view name, bool enable)
	{
		auto* panel = GetPanelDesc(name);

		if (panel != nullptr)
			panel->IsOpen = enable;
	}

	Ref<Panel> PanelManager::GetPanel(std::string_view name)
	{
		auto* panel = GetPanelDesc(name);

		return panel == nullptr ? nullptr : panel->PanelRef;
	}

	PanelDescription* PanelManager::GetPanelDesc(std::string_view name)
	{
		for (auto& panel : m_Panels)
		{
			if (panel.PanelRef->GetName() == name)
			{
				return &panel;
			}
		}

		ATN_CORE_ERROR("PanelManager: Failed to find panel '{0}'", name);
		return nullptr;
	}

	bool PanelManager::OnKeyPressedEvent(KeyPressedEvent& event)
	{
		bool ctrl = Input::IsKeyPressed(Keyboard::LCtrl) || Input::IsKeyPressed(Keyboard::RCtrl);

		if (ctrl)
		{
			auto keycode = event.GetKeyCode();
			for (auto& panel : m_Panels)
			{
				ATN_CORE_TRACE(event.ToString());
				if (panel.HotKey != Keyboard::Escape && panel.HotKey == keycode)
				{
					panel.IsOpen = !panel.IsOpen;
				}
			}
		}

		return false;
	}
}
