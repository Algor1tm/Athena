#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class Titlebar
	{
	public:
		Titlebar(const String& name);

		void OnImGuiRender();

		bool IsHovered() { return m_Hovered; }
		float GetHeight() { return m_Height; }

	private:
		String m_Name;
		bool m_Hovered;
		const float m_Height = 58.f;
	};
}
