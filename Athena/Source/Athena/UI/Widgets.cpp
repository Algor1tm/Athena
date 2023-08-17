#include "Widgets.h"


namespace Athena::UI
{
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

	bool TreeNode(std::string_view label, bool defaultOpen)
	{
		ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_AllowItemOverlap |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_Framed |
			ImGuiTreeNodeFlags_FramePadding;

		if (defaultOpen)
			flags |= ImGuiTreeNodeFlags_DefaultOpen;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 3, 5 });
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
		bool result = ImGui::TreeNodeEx(label.data(), flags, label.data());
		ImGui::PopStyleVar(2);

		return result;
	}

	void TreePop()
	{
		ImGui::TreePop();
	}

	bool BeginPropertyTable()
	{
		const ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable |
			ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_SizingFixedFit;

		UI::ShiftCursorY(1.f);
		return ImGui::BeginTable("table", 2, tableFlags);
	}

	void EndPropertyTable()
	{
		ImGui::EndTable();
	}

	void Property(std::string_view label, float height)
	{
		static float offset = 15.f;

		ImGui::TableNextRow(ImGuiTableRowFlags_None, height + offset);

		ImGui::TableSetColumnIndex(0);
		ShiftCursorY((height + offset - ImGui::GetTextLineHeight()) / 2.f - 2.f);
		ImGui::Text(label.data());

		ImGui::TableSetColumnIndex(1);

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 15.f);
		ShiftCursorY(offset / 2.f);
	}

	void Property(std::string_view label)
	{
		float height = ImGui::GetFrameHeight();
		Property(label, height);
	}

	bool ComboBox(std::string_view label, const std::string_view* elems, uint32 elemsNum, std::string_view* selectedElem)
	{
		bool itemSelected = false;

		if (ImGui::BeginCombo(label.data(), (*selectedElem).data()))
		{
			for (uint32 i = 0; i < elemsNum; ++i)
			{
				std::string_view elem = elems[i];
				bool isSelected = elem == *selectedElem;
				bool changeItemTheme = isSelected;

				if (changeItemTheme)
				{
					ImVec4& color = ImGui::GetStyle().Colors[ImGuiCol_ResizeGripActive];
					ImGui::PushStyleColor(ImGuiCol_Header, color);
					ImGui::PushStyleColor(ImGuiCol_HeaderActive, color);
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color);
				}

				if (ImGui::Selectable(elem.data(), &isSelected))
				{
					*selectedElem = elem;
					itemSelected = true;
				}

				if (changeItemTheme)
					ImGui::PopStyleColor(3);

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		return itemSelected;
	}

	bool ComboBox(std::string_view label, const String* elems, uint32 elemsNum, String* selectedElem)
	{
		std::vector<std::string_view> elemViews(elemsNum);
		for (uint32 i = 0; i < elemsNum; ++i)
			elemViews[i] = elems[i];

		std::string_view selectedElemView = *selectedElem;

		bool result = ComboBox(label, elemViews.data(), elemsNum, &selectedElemView);

		if(result)
			*selectedElem = selectedElemView;

		return result;
	}
}
