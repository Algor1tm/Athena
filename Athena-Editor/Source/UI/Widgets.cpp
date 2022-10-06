#include "UI/Widgets.h"

#include <ImGui/imgui_internal.h>


namespace Athena::UI
{
	void DrawVec3Controller(std::string_view label, Vector3& values, float defaultValues, float height)
	{
		DrawController(label, height, [label, defaultValues, &values]()
			{
				ImGui::PushID(label.data());

				float full_width = ImGui::GetContentRegionAvail().x - 15.f;
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

				float buttonWidth = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.f;
				ImVec2 buttonSize = { buttonWidth, buttonWidth };

				float dragWidth = (full_width - 3 * buttonWidth) / 3.f;

				ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.1f, 0.15f, 1.f });
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.2f, 0.2f, 1.f });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.8f, 0.1f, 0.15f, 1.f });
				UI::PushBoldFont();
				if (ImGui::Button("X", buttonSize))
					values.x = defaultValues;
				ImGui::PopFont();

				ImGui::SameLine();
				ImGui::PushItemWidth(dragWidth);
				ImGui::DragFloat("##X", &values.x, 0.07f, 0.f, 0.f, "%.2f");
				ImGui::PopItemWidth();
				ImGui::SameLine();

				ImGui::PopStyleColor(3);

				ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.7f, 0.2f, 1.f });
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.3f, 0.8f, 0.3f, 1.f });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.2f, 0.7f, 0.2f, 1.f });
				UI::PushBoldFont();
				if (ImGui::Button("Y", buttonSize))
					values.y = defaultValues;
				ImGui::PopFont();

				ImGui::SameLine();
				ImGui::PushItemWidth(dragWidth);
				ImGui::DragFloat("##Y", &values.y, 0.07f, 0.f, 0.f, "%.2f");
				ImGui::PopItemWidth();
				ImGui::SameLine();

				ImGui::PopStyleColor(3);

				ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f, 0.25f, 0.8f, 1.f });
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.2f, 0.35f, 0.9f, 1.f });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f, 0.25f, 0.8f, 1.f });
				UI::PushBoldFont();
				if (ImGui::Button("Z", buttonSize))
					values.z = defaultValues;
				ImGui::PopFont();

				ImGui::SameLine();
				ImGui::PushItemWidth(dragWidth);
				ImGui::DragFloat("##Z", &values.z, 0.7f, 0.f, 0.f, "%.2f");
				ImGui::PopItemWidth();

				ImGui::PopStyleColor(3);

				ImGui::PopStyleVar();

				ImGui::PopID();

				return false;
			});

	}

	bool TextInput(const String& label, String& destination, ImGuiInputTextFlags flags)
	{
		static char buffer[128];
		memset(buffer, 0, sizeof(buffer));
		strcpy_s(buffer, label.c_str());
		if (ImGui::InputText("##TextInput", buffer, sizeof(buffer), flags))
		{
			destination = String(buffer);
			return true;
		}

		return false;
	}

	bool TextInputWithHint(const std::string_view hint, String& destination, ImGuiInputTextFlags flags)
	{
		static char buffer[128];
		memset(buffer, 0, sizeof(buffer));
		if (ImGui::InputTextWithHint("##TextInputWithHint", hint.data(), buffer, sizeof(buffer), flags))
		{
			destination = String(buffer);
			return true;
		}

		return false;
	}

	bool BeginDrawControllers()
	{
		static const ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable | 
			ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_SizingFixedFit;

		UI::ShiftCursorY(1.f);
		return ImGui::BeginTable("table", 2, flags);
	}

	void EndDrawControllers()
	{
		ImGui::EndTable();
	}
}
