#pragma once

#include <ImGui/imgui.h>


namespace Athena::UI
{
	void PushBoldFont();

	inline void ShiftCursorX(float offset)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
	}

	inline void ShiftCursorY(float offset)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
	}
}
