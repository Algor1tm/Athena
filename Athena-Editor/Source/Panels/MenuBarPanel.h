#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Texture.h"
#include "Athena/Scene/Scene.h"

#include "Panel.h"

#include <functional>


namespace Athena
{
	struct MenuItem
	{
		std::string_view Label;
		bool Selected = false;

		std::function<void()> Callback;
	};

	struct MenuButton
	{
		Ref<Texture2D> Icon;

		std::function<void(Ref<Texture2D>&)> Callback;
	};


	class MenuBarPanel : public Panel
	{
	public:
		MenuBarPanel(std::string_view name);

		virtual void OnImGuiRender() override;

		void SetSceneRef(const Ref<Scene>& scene) { m_Scene = scene; };
		void SetLogoIcon(const Ref<Texture2D>& logo) { m_Logo = logo; };

		void AddMenuItem(std::string_view text, const std::function<void()>& callback);
		void AddMenuButton(const Ref<Texture2D>& icon, const std::function<void(Ref<Texture2D>&)>& callback);

		void UseWindowDefaultButtons(bool enabled) { m_UseWindowDefaultButtons = enabled; }

	private:
		uint64 m_SelectedLabelIndex = -1;
		std::vector<MenuItem> m_MenuItems;
		std::vector<MenuButton> m_MenuButtons;

		bool m_UseWindowDefaultButtons = false;
		Ref<Texture2D> m_CloseButton;
		Ref<Texture2D> m_MinimizeButton;
		Ref<Texture2D> m_RestoreDownButton;
		Ref<Texture2D> m_MaximizeButton;

		Ref<Scene> m_Scene;
		Ref<Texture2D> m_Logo;
	};
}
