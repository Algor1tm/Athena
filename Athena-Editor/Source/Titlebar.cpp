#include "Titlebar.h"

#include "Athena/Core/Application.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>


namespace Athena
{
	void Titlebar::OnImGuiRender()
	{
        const bool isMaximized = Application::Get().GetWindow().GetWindowMode() == WindowMode::Maximized;

        float titlebarVerticalOffset = isMaximized ? -6.0f : 0.0f;
        const ImVec2 windowPadding = isMaximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f);

        static float moveOffsetX;
        static float moveOffsetY;
        const float w = ImGui::GetContentRegionAvail().x;
        const float buttonsAreaWidth = 94;

        // Title bar drag area
        const ImVec2 titlebarMin = ImGui::GetCursorScreenPos();
        const ImVec2 titlebarMax = { ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - windowPadding.y * 2.0f,
                                     ImGui::GetCursorScreenPos().y + m_Height };
        auto* bgDrawList = ImGui::GetBackgroundDrawList();
        auto* fgDrawList = ImGui::GetForegroundDrawList();
        bgDrawList->AddRectFilled(titlebarMin, titlebarMax, IM_COL32(21, 21, 21, 255));

        ImGui::BeginHorizontal("Titlebar", { ImGui::GetWindowWidth() - windowPadding.y * 2.0f, ImGui::GetFrameHeightWithSpacing() });

        ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset)); // Reset cursor pos
        // DEBUG DRAG BOUNDS
        fgDrawList->AddRect(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetCursorScreenPos().x + w - buttonsAreaWidth, ImGui::GetCursorScreenPos().y + m_Height), IM_COL32(222, 43, 43, 255));
        ImGui::InvisibleButton("##TitlebarDragZone", ImVec2(w - buttonsAreaWidth, m_Height));

        m_Hovered = ImGui::IsItemHovered();

        if (!isMaximized)
        {
            float windowMousePosY = ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y;
            if (windowMousePosY >= 0.0f && windowMousePosY <= 5.0f)
                m_Hovered = true; // Account for the top-most pixels which don't register
        }

        ImGui::EndHorizontal();
	}
}
