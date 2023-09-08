#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Texture.h"


namespace Athena
{
	class Titlebar
	{
	public:
		Titlebar(const String& name);

		void OnImGuiRender();
		void SetMenubarCallback(const std::function<void()>& callback) { m_MenubarCallback = callback; }

		bool IsHovered() { return m_Hovered; }
		float GetHeight() { return m_Height; }

	private:
		String m_Name;
		bool m_Hovered;
		const float m_Height = 58.f;

		std::function<void()> m_MenubarCallback;
	};
}
