#include "UI.h"


namespace Athena::UI
{
	void PushFont(Fonts font)
	{
		ImGuiIO& io = ImGui::GetIO();
		ImFont* imguiFont = io.FontDefault;

		switch (font)
		{
		case Fonts::Default: imguiFont = io.FontDefault; break;
		case Fonts::Bold: imguiFont = io.Fonts->Fonts[1]; break;
		case Fonts::Default22: imguiFont = io.Fonts->Fonts[2]; break;
		}

		ImGui::PushFont(imguiFont);
	}

	void PopFont()
	{
		ImGui::PopFont();
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

	ImColor MultiplyColorByScalar(const ImColor& color, float scalar)
	{
		const ImVec4& colRow = color.Value;
		float hue, sat, val;
		ImGui::ColorConvertRGBtoHSV(colRow.x, colRow.y, colRow.z, hue, sat, val);
		return ImColor::HSV(hue, sat, Math::Min(val * scalar, 1.0f));
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

	void PropertyImage(std::string_view label) 
	{
		const float height = 50.f;
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

	void ButtonImage(const Ref<Texture>& imageNormal, const Ref<Texture>& imageHovered, const Ref<Texture>& imagePressed,
		ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed,
		ImVec2 rectMin, ImVec2 rectMax)
	{
		auto* drawList = ImGui::GetWindowDrawList();
		if (ImGui::IsItemActive())
			drawList->AddImage(imagePressed->GetRendererID(), rectMin, rectMax, ImVec2(0, 1), ImVec2(1, 0), tintPressed);
		else if (ImGui::IsItemHovered())
			drawList->AddImage(imageHovered->GetRendererID(), rectMin, rectMax, ImVec2(0, 1), ImVec2(1, 0), tintHovered);
		else
			drawList->AddImage(imageNormal->GetRendererID(), rectMin, rectMax, ImVec2(0, 1), ImVec2(1, 0), tintNormal);
	}

	void ButtonImage(const Ref<Texture>& image, ImU32 tintNormal, ImU32 tintHovered, ImU32 tintPressed)
	{
		ButtonImage(image, image, image, tintNormal, tintHovered, tintPressed, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	void Image(Ref<Texture2D> image, const ImVec2& size, const ImVec4& tint_col, const ImVec4& border_col)
	{
		ImGui::Image(image->GetRendererID(), size, { 0, 1 }, { 1, 0 }, tint_col, border_col);
	}

	bool BeginMenubar(const ImRect& barRectangle)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;
		/*if (!(window->Flags & ImGuiWindowFlags_MenuBar))
			return false;*/

		ImGuiContext& g = *GImGui;

		IM_ASSERT(!window->DC.MenuBarAppending);
		ImGui::BeginGroup(); // Backup position on layer 0 // FIXME: Misleading to use a group for that backup/restore
		ImGui::PushID("##menubar");

		const ImVec2 padding = window->WindowPadding;

		// We don't clip with current window clipping rectangle as it is already set to the area below. However we clip with window full rect.
		// We remove 1 worth of rounding to Max.x to that text in long menus and small windows don't tend to display over the lower-right rounded area, which looks particularly glitchy.
		ImRect bar_rect = barRectangle;
		bar_rect.Min.y += padding.y;
		bar_rect.Max.y += padding.y;
		ImRect clip_rect(IM_ROUND(ImMax(window->Pos.x, bar_rect.Min.x + window->WindowBorderSize + window->Pos.x - 10.0f)), IM_ROUND(bar_rect.Min.y + window->WindowBorderSize + window->Pos.y),
			IM_ROUND(ImMax(bar_rect.Min.x + window->Pos.x, bar_rect.Max.x - ImMax(window->WindowRounding, window->WindowBorderSize))), IM_ROUND(bar_rect.Max.y + window->Pos.y));

		clip_rect.ClipWith(window->OuterRectClipped);
		ImGui::PushClipRect(clip_rect.Min, clip_rect.Max, false);

		// We overwrite CursorMaxPos because BeginGroup sets it to CursorPos (essentially the .EmitItem hack in EndMenuBar() would need something analogous here, maybe a BeginGroupEx() with flags).
		window->DC.CursorPos = window->DC.CursorMaxPos = ImVec2(bar_rect.Min.x + window->Pos.x, bar_rect.Min.y + window->Pos.y);
		window->DC.LayoutType = ImGuiLayoutType_Horizontal;
		window->DC.IsSameLine = false;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Menu;
		window->DC.MenuBarAppending = true;

		window->DC.CurrLineSize.y = ImMax(window->DC.CurrLineSize.y, g.FontSize + g.Style.FramePadding.y * 2);
		window->DC.CurrLineTextBaseOffset = ImMax(window->DC.CurrLineTextBaseOffset, g.Style.FramePadding.y);

		return true;
	}

	void EndMenubar()
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;
		ImGuiContext& g = *GImGui;

		// Nav: When a move request within one of our child menu failed, capture the request to navigate among our siblings.
		if (ImGui::NavMoveRequestButNoResultYet() && (g.NavMoveDir == ImGuiDir_Left || g.NavMoveDir == ImGuiDir_Right) && (g.NavWindow->Flags & ImGuiWindowFlags_ChildMenu))
		{
			// Try to find out if the request is for one of our child menu
			ImGuiWindow* nav_earliest_child = g.NavWindow;
			while (nav_earliest_child->ParentWindow && (nav_earliest_child->ParentWindow->Flags & ImGuiWindowFlags_ChildMenu))
				nav_earliest_child = nav_earliest_child->ParentWindow;
			if (nav_earliest_child->ParentWindow == window && nav_earliest_child->DC.ParentLayoutType == ImGuiLayoutType_Horizontal && (g.NavMoveFlags & ImGuiNavMoveFlags_Forwarded) == 0)
			{
				// To do so we claim focus back, restore NavId and then process the movement request for yet another frame.
				// This involve a one-frame delay which isn't very problematic in this situation. We could remove it by scoring in advance for multiple window (probably not worth bothering)
				const ImGuiNavLayer layer = ImGuiNavLayer_Menu;
				IM_ASSERT(window->DC.NavLayersActiveMaskNext & (1 << layer)); // Sanity check
				ImGui::FocusWindow(window);
				ImGui::SetNavID(window->NavLastIds[layer], layer, 0, window->NavRectRel[layer]);
				g.NavDisableHighlight = true; // Hide highlight for the current frame so we don't see the intermediary selection.
				g.NavDisableMouseHover = g.NavMousePosDirty = true;
				ImGui::NavMoveRequestForward(g.NavMoveDir, g.NavMoveClipDir, g.NavMoveFlags, g.NavMoveScrollFlags); // Repeat
			}
		}

		IM_MSVC_WARNING_SUPPRESS(6011); // Static Analysis false positive "warning C6011: Dereferencing NULL pointer 'window'"
		IM_ASSERT(window->DC.MenuBarAppending);
		ImGui::PopClipRect();
		ImGui::PopID();
		window->DC.MenuBarOffset.x = window->DC.CursorPos.x - window->Pos.x; // Save horizontal position so next append can reuse it. This is kinda equivalent to a per-layer CursorPos.
		g.GroupStack.back().EmitItem = false;
		ImGui::EndGroup(); // Restore position on layer 0
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		window->DC.IsSameLine = false;
		window->DC.NavLayerCurrent = ImGuiNavLayer_Main;
		window->DC.MenuBarAppending = false;
	}

	bool ButtonCentered(std::string_view label, const ImVec2& size)
	{
		ImGuiStyle& style = ImGui::GetStyle();

		float actualSize = ImGui::CalcTextSize(label.data()).x + style.FramePadding.x * 2.0f;
		float avail = ImGui::GetContentRegionAvail().x;

		float off = (avail - actualSize) * 0.5f;
		if (off > 0.0f)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

		return ImGui::Button(label.data(), size);
	}

	void TextCentered(std::string_view label)
	{
		ImGuiStyle& style = ImGui::GetStyle();

		float actualSize = ImGui::CalcTextSize(label.data()).x + style.FramePadding.x * 2.0f;
		float avail = ImGui::GetContentRegionAvail().x;

		float off = (avail - actualSize) * 0.5f;
		if (off > 0.0f)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

		return ImGui::Text(label.data());
	}

	void InvisibleItem(std::string_view strID, ImVec2 itemSize)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		const ImGuiID id = window->GetID(strID.data());
		ImVec2 size = ImGui::CalcItemSize(itemSize, 0.0f, 0.0f);
		const ImRect bb(window->DC.CursorPos, { window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y });
		ImGui::ItemSize(size);
		ImGui::ItemAdd(bb, id);
	}
}
