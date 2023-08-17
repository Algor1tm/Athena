#include "Utils.h"

#include <ImGui/imgui_internal.h>


namespace Athena::UI
{
	void PushBoldFont()
	{
		ImGuiIO& io = ImGui::GetIO();
		auto& boldFont = io.Fonts->Fonts[0];

		ImGui::PushFont(boldFont);
	}

	void ShiftCursorX(float offset)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
	}

	void ShiftCursorY(float offset)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
	}

	void ShiftCursor(float offsetX, float offsetY)
	{
		ShiftCursorX(offsetX);
		ShiftCursorY(offsetY);
	}

	const ImVec4& GetDarkColor()
	{
		return ImGui::GetStyle().Colors[ImGuiCol_TitleBg];
	}
}
