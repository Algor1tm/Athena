#pragma once

#include "Athena/Math/Vector.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <string_view>


namespace Athena::UI
{
	void DrawVec3Control(std::string_view label, Vector3& values, float defaultValues, float columnWidth = 70.f);

	template <typename Controller>
	void DrawController(std::string_view label, float columnWidth, Controller controller)
	{
		ImGui::PushID(label.data());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.data());
		ImGui::NextColumn();

		controller();

		ImGui::Columns(1);
		ImGui::PopID();
	}
}
