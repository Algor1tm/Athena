#pragma once


namespace Athena
{
	class Titlebar
	{
	public:

		void OnImGuiRender();

		bool IsHovered() { return m_Hovered; }
		float GetHeight() { return m_Height; }

	private:
		bool m_Hovered;
		const float m_Height = 58.f;
	};
}
