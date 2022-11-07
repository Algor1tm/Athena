#include "MenuBarPanel.h"

#include "Athena/Core/Application.h"

#include "UI/Widgets.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>


namespace Athena
{
    MenuBarPanel::MenuBarPanel(std::string_view name)
        : Panel(name)
	{
		m_CloseButton = Texture2D::Create("Resources/Icons/Editor/MenuBar/CloseButton.png");
		m_MinimizeButton = Texture2D::Create("Resources/Icons/Editor/MenuBar/MinimizeButton.png");
		m_RestoreDownButton = Texture2D::Create("Resources/Icons/Editor/MenuBar/RestoreDownButton.png");
		m_MaximizeButton = Texture2D::Create("Resources/Icons/Editor/MenuBar/MaximizeButton.png");
	}

    void MenuBarPanel::AddMenuItem(std::string_view text, const std::function<void()>& callback)
    {
        m_MenuItems.push_back({ text, false, callback });
    }

    void MenuBarPanel::AddMenuButton(const Ref<Texture2D>& icon, const std::function<void(Ref<Texture2D>&)>& callback)
    {
        m_MenuButtons.push_back({ icon, callback });
	}

	void MenuBarPanel::OnImGuiRender()
	{
        const ImVec2 framePadding = { 15, 1 };
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, framePadding);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });

        ImGui::PushStyleColor(ImGuiCol_WindowBg, UI::GetDarkColor());
        ImGui::Begin("##Menubar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleColor();

        float window_height = ImGui::GetWindowHeight();

        ///////// Logo ////////////////
        if (m_Logo != nullptr)
        {
            float scale = 0.85f;
            ImGui::SetCursorPos({(1.f - scale) * window_height / 2.f, (1.f - scale) * window_height / 2.f });
            ImGui::Image(m_Logo->GetRendererID(), { scale * window_height, scale * window_height }, { 0, 1 }, { 1, 0 });
            ImGui::SetCursorPos({window_height, 0});
        }

        ///////// Menu Items ////////////////
        ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyle().Colors[ImGuiCol_ResizeGripActive]);
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.5f));
        float buttonSizeY = window_height / 2.5f;
        ImVec2 menuStart = ImGui::GetCursorPos();
        ImVec2 popupPos = { ImGui::GetWindowPos().x + menuStart.x , ImGui::GetWindowPos().y + menuStart.y + buttonSizeY };
        for (SIZE_T i = 0; i < m_MenuItems.size(); ++i)
        {
            ImVec2 textSize = ImGui::CalcTextSize(m_MenuItems[i].Label.data());
            ImVec2 itemSize = { textSize.x + framePadding.x * 2.0f, buttonSizeY };
            if(i <= m_SelectedLabelIndex)
                popupPos.x += itemSize.x; 
            if (ImGui::Selectable(m_MenuItems[i].Label.data(), &m_MenuItems[i].Selected, 0, itemSize))
            {
                ImGui::OpenPopup(m_MenuItems[i].Label.data());
                m_SelectedLabelIndex = i;
                popupPos.x -= itemSize.x;
            }
            else if (m_MenuItems[i].Selected)
            {
                popupPos.x -= itemSize.x;
            }
            ImGui::SameLine();
        }
        ImGui::PopStyleColor();

        ///////// Scene Name ////////////////
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(46.f / 255.f, 44.f / 255.f, 44.f / 255.f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(46.f / 255.f, 44.f / 255.f, 44.f / 255.f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(53.f / 255.f, 51.f / 255.f, 51.f / 255.f, 1.0f));
       
        if (m_Scene != nullptr)
        {
            ImVec2 textSize = ImGui::CalcTextSize(m_Scene->GetSceneName().data());
            ImVec2 itemSize = { textSize.x + framePadding.x * 2.0f, buttonSizeY };
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetWindowSize().x * 0.01f);
            if (ImGui::Button(m_Scene->GetSceneName().c_str(), itemSize))
                ImGui::OpenPopup("SceneName");
        }

        ImGui::PopStyleVar(6);
        ImGui::PopStyleColor(2);


        if (m_SelectedLabelIndex != -1)
        {
            ImGui::SetNextWindowPos(popupPos);
            if (ImGui::BeginPopup(m_MenuItems[m_SelectedLabelIndex].Label.data()))
            {
                m_MenuItems[m_SelectedLabelIndex].Callback();
                ImGui::EndPopup();
            }
            else
            {
                m_MenuItems[m_SelectedLabelIndex].Selected = false;
                m_SelectedLabelIndex = -1;
            }
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 10.f, 10.f });
        if (ImGui::BeginPopup("SceneName"))
        {
            String newName;
            if (m_Scene != nullptr && UI::TextInput(m_Scene->GetSceneName(), newName))
                m_Scene->SetSceneName(newName);
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();

        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(46.f / 255.f, 44.f / 255.f, 44.f / 255.f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

        ///////// Menu Buttons ////////////////
        float size = window_height / 2.5f;
        for (SIZE_T i = 0; i < m_MenuButtons.size(); ++i)
        {
            ImGui::SetCursorPos({ ImGui::GetContentRegionMax().x * 0.5f - (size * 0.5f) + i * size * 1.25f, window_height - size - 5.f });
            if (ImGui::ImageButton(m_MenuButtons[i].Icon->GetRendererID(), { size, size }, { 0, 1 }, {1, 0}))
            {
                m_MenuButtons[i].Callback(m_MenuButtons[i].Icon);
            }
        }

        ///////// Window Buttons ////////////////
        size = window_height / 1.7f;
		if (m_UseWindowDefaultButtons)
		{
            ImGui::SameLine();
            {
                ImGui::SetCursorPos({ ImGui::GetContentRegionMax().x - size - 10.f, 0.f });
                if (ImGui::ImageButton(m_CloseButton->GetRendererID(), { size, size * 3.f / 4.f }, { 0, 1 }, { 1, 0 }))
                {
                    Application::Get().Close();
                }
            }
            ImGui::SameLine();
            {
                ImGui::SetCursorPos({ ImGui::GetContentRegionMax().x - 2 * size - 10.f, 0.f });
                auto mode = Application::Get().GetWindow().GetWindowMode();
                if (mode != WindowMode::Default)
                {
                    if (ImGui::ImageButton(m_RestoreDownButton->GetRendererID(), { size, size * 3.f / 4.f }, { 0, 1 }, { 1, 0 }))
                    {
                        auto& window = Application::Get().GetWindow();
                        window.SetWindowMode(window.GetWindowMode() == WindowMode::Fullscreen ? WindowMode::Maximized : WindowMode::Default);;
                    }
                }
                else
                {
                    if (ImGui::ImageButton(m_MaximizeButton->GetRendererID(), { size, size * 3.f / 4.f }, { 0, 1 }, { 1, 0 }))
                    {
                        Application::Get().GetWindow().SetWindowMode(WindowMode::Maximized);;
                    }
                }
            }
            ImGui::SameLine();
            {
                ImGui::SetCursorPos({ ImGui::GetContentRegionMax().x - 3 * size - 10.f, 0.f });

                if (ImGui::ImageButton(m_MinimizeButton->GetRendererID(), { size, size * 3.f / 4.f }, { 0, 1 }, { 1, 0 }))
                {
                    Application::Get().GetWindow().SetWindowMode(WindowMode::Minimized);;
                }
            }
		}
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();

        ImGui::End();
	}
}
