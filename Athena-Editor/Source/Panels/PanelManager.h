#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Input/Event.h"
#include "Athena/Input/KeyEvent.h"
#include "Athena/Input/Keyboard.h"

#include "Panels/Panel.h"

#include <unordered_map>


#define SCENE_HIERARCHY_PANEL_ID "SceneHierarchy"
#define CONTENT_BORWSER_PANEL_ID "ContentBrowser"
#define PROFILING_PANEL_ID		 "Profiling"
#define VIEWPORT_PANEL_ID		 "Viewport"
#define SETTINGS_PANEL_ID		 "Settings"


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
		static void Shutdown();

		static void OnImGuiRender();
		static void OnEvent(Event& event);

		static void ImGuiRenderAsMenuItems();

		static void AddPanel(const Ref<Panel>& panel, bool isHideable = true);
		static void AddPanel(const Ref<Panel>& panel, Keyboard::Key hotkey);

		template<typename T>
		static Ref<T> GetPanel(std::string_view name)
		{
			return m_Panels.at(name).PanelRef.As<T>();
		}

	private:
		static bool OnKeyPressedEvent(KeyPressedEvent& event);

	private:
		static std::unordered_map<std::string_view, PanelDescription> m_Panels;
	};
}
