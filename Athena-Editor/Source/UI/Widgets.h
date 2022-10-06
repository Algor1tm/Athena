#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Core/Color.h"
#include "Athena/Renderer/Texture.h"

#include "Utils.h"

#include <ImGui/imgui.h>

#include <string_view>


namespace Athena::UI
{
	void DrawVec3Controller(std::string_view label, Vector3& values, float defaultValues, float height);
	
	bool TextInput(const String& label, String& destination, ImGuiInputTextFlags flags = 0);
	bool TextInputWithHint(const std::string_view hint, String& destination, ImGuiInputTextFlags flags = 0);

	bool BeginDrawControllers();
	void EndDrawControllers();

	template <typename Controller>
	bool DrawController(std::string_view label, float height, Controller controller)
	{
		static float offset = 15.f;

		ImGui::TableNextRow(ImGuiTableRowFlags_None, height + offset);

		ImGui::TableSetColumnIndex(0);
		ShiftCursorY((height + offset - ImGui::GetTextLineHeight()) / 2.f);
		ImGui::Text(label.data());

		ImGui::TableSetColumnIndex(1);
		
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 15.f);
		ShiftCursorY(offset / 2.f);
		bool result = controller();

		return result;
	}

	template <typename ImGuiWidget>
	bool DrawImGuiWidget(std::string_view label, ImGuiWidget widget)
	{
		ImGui::Text(label.data());
		ImGui::SameLine();

		return widget();
	}


	template <typename CallbackFunc>
	void Selectable(std::string_view label, bool* isSelected, CallbackFunc callback)
	{
		if (*isSelected)
		{
			ImVec4& color = ImGui::GetStyle().Colors[ImGuiCol_ResizeGripActive];
			ImGui::PushStyleColor(ImGuiCol_Header, color);
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, color);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color);
		}

		if (ImGui::Selectable(label.data(), isSelected))
		{
			callback();
		}

		if(*isSelected)
			ImGui::PopStyleColor(3);
	}
}
