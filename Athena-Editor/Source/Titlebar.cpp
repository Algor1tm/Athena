#include "Titlebar.h"

#include "Athena/Core/Application.h"
#include "Athena/UI/Widgets.h"

#include "EditorLayer.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>


namespace Athena
{
    Titlebar::Titlebar(const String& name)
    {
        m_Name = name;

        const FilePath& resources = EditorLayer::Get().GetConfig().EditorResources;

        m_Icons["Logo"] = Texture2D::Create(resources / "Icons/Editor/MenuBar/LogoWhite.png");

        m_Icons["Close"] = Texture2D::Create(resources / "Icons/Editor/MenuBar/CloseButton.png");
        m_Icons["Minimize"] = Texture2D::Create(resources / "Icons/Editor/MenuBar/MinimizeButton.png");
        m_Icons["Restore"] = Texture2D::Create(resources / "Icons/Editor/MenuBar/RestoreButton.png");
        m_Icons["Maximize"] = Texture2D::Create(resources / "Icons/Editor/MenuBar/MaximizeButton.png");
    }

	void Titlebar::OnImGuiRender()
	{
        const bool isMaximized = Application::Get().GetWindow().GetWindowMode() == WindowMode::Maximized;

        float titlebarVerticalOffset = isMaximized ? -6.0f : 0.0f;
        const ImVec2 windowPadding = isMaximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f); // ImGui::GetCurrentWindow()->WindowPadding

        static float moveOffsetX;
        static float moveOffsetY;
        const float w = ImGui::GetContentRegionAvail().x;
        const float buttonsAreaWidth = 94;

        // Title bar drag area
        ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset));
        const ImVec2 titlebarMin = ImGui::GetCursorScreenPos();
        const ImVec2 titlebarMax = { ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth() - windowPadding.y * 2.0f,
                                     ImGui::GetCursorScreenPos().y + m_Height };
        auto* bgDrawList = ImGui::GetBackgroundDrawList();
        auto* fgDrawList = ImGui::GetForegroundDrawList();
        bgDrawList->AddRectFilled(titlebarMin, titlebarMax, IM_COL32(21, 21, 21, 255));

        // Logo
        {
            const int logoWidth = 48;
            const int logoHeight = 48;
            const ImVec2 logoOffset(16.0f + windowPadding.x, 5.0f + windowPadding.y + titlebarVerticalOffset);
            const ImVec2 logoRectStart = { ImGui::GetItemRectMin().x + logoOffset.x, ImGui::GetItemRectMin().y + logoOffset.y };
            const ImVec2 logoRectMax = { logoRectStart.x + logoWidth, logoRectStart.y + logoHeight };
            fgDrawList->AddImage(m_Icons.at("Logo")->GetRendererID(), logoRectStart, logoRectMax, { 0, 1 }, { 1, 0 });
        }

        ImGui::BeginHorizontal("Titlebar", { ImGui::GetWindowWidth() - windowPadding.y * 2.0f, ImGui::GetFrameHeightWithSpacing() });

        ImGui::SetCursorPos(ImVec2(windowPadding.x, windowPadding.y + titlebarVerticalOffset)); // Reset cursor pos
        // DEBUG DRAG BOUNDS
        //fgDrawList->AddRect(ImGui::GetCursorScreenPos(), ImVec2(ImGui::GetCursorScreenPos().x + w - buttonsAreaWidth, ImGui::GetCursorScreenPos().y + m_Height), IM_COL32(222, 43, 43, 255));

        UI::InvisibleItem("##TitlebarDragZone", ImVec2(w - buttonsAreaWidth, m_Height));
        m_Hovered = ImGui::IsItemHovered();

        if (isMaximized)
        {
            float windowMousePosY = ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y;
            if (windowMousePosY >= 0.0f && windowMousePosY <= 5.0f)
                m_Hovered = true; // Account for the top-most pixels which don't register
        }

        // Draw Menubar
        if (m_MenubarCallback)
        {
            ImGui::SuspendLayout();
            {
                ImGui::SetItemAllowOverlap();
                const float logoHorizontalOffset = 16.0f * 2.0f + 48.0f + windowPadding.x;
                ImGui::SetCursorPos(ImVec2(logoHorizontalOffset, 6.0f + titlebarVerticalOffset));

                const ImRect menuBarRect = { ImGui::GetCursorPos(), { ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x, ImGui::GetFrameHeightWithSpacing() } };

                ImGui::BeginGroup();
                if (UI::BeginMenubar(menuBarRect))
                {
                    m_MenubarCallback();
                }

                UI::EndMenubar();
                ImGui::EndGroup();

                if (ImGui::IsItemHovered())
                    m_Hovered = false;
            }

            ImGui::ResumeLayout();
        }

        {
            // Centered Window title
            ImVec2 currentCursorPos = ImGui::GetCursorPos();
            ImVec2 textSize = ImGui::CalcTextSize(m_Name.c_str());
            ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.5f - textSize.x * 0.5f, 2.0f + windowPadding.y + 2.0f));

            UI::PushFont(UI::Fonts::Default22);
            ImGui::Text("%s", m_Name.c_str()); // Draw title
            UI::PopFont();

            ImGui::SetCursorPos(currentCursorPos);
        }


        // Window buttons
        const ImColor buttonDefault = IM_COL32(192, 192, 192, 255);
        const ImU32 buttonColN = UI::MultiplyColorByScalar(buttonDefault, 0.9f);
        const ImU32 buttonColH = UI::MultiplyColorByScalar(buttonDefault, 1.2f);
        const ImU32 buttonColP = IM_COL32(128, 128, 128, 255);
        const float buttonWidth = 14.0f;
        const float buttonHeight = 14.0f;

        auto& window = Application::Get().GetWindow();

        // Minimize Button

        ImGui::Spring();
        UI::ShiftCursorY(8.0f);
        {
            if (ImGui::InvisibleButton("Minimize", ImVec2(buttonWidth, buttonHeight)))
            {
                window.SetWindowMode(WindowMode::Minimized);
            }

            UI::ButtonImage(m_Icons.at("Minimize"), buttonColN, buttonColH, buttonColP);
        }


        // Maximize Button
        ImGui::Spring(-1.0f, 17.0f);
        UI::ShiftCursorY(8.0f);
        {
            if (ImGui::InvisibleButton("Maximize", ImVec2(buttonWidth, buttonHeight)))
            {
                if (isMaximized)
                    window.SetWindowMode(WindowMode::Default);
                else
                    window.SetWindowMode(WindowMode::Maximized);
            }

            UI::ButtonImage(isMaximized ? m_Icons.at("Restore") : m_Icons.at("Maximize"), buttonColN, buttonColH, buttonColP);
        }

        // Close Button
        ImGui::Spring(-1.0f, 15.0f);
        UI::ShiftCursorY(8.0f);
        {
            if (ImGui::InvisibleButton("Close", ImVec2(buttonWidth, buttonHeight)))
            {
                Application::Get().Close();
            }

            UI::ButtonImage(m_Icons.at("Close"), buttonDefault, UI::MultiplyColorByScalar(buttonDefault, 1.4f), buttonColP);
        }

        ImGui::Spring(-1.0f, 18.0f);

        ImGui::EndHorizontal();
	}
}
