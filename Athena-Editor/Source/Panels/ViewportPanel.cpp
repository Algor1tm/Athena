#include "ViewportPanel.h"

#include "Athena/Core/Application.h"
#include "Athena/UI/UI.h"
#include "ImGuizmoLayer.h"

#include <ImGui/imgui.h>


namespace Athena
{
    ViewportPanel::ViewportPanel(std::string_view name, const Ref<EditorContext>& context)
        : Panel(name, context)
    {

    }

	void ViewportPanel::OnImGuiRender()
	{
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
        ImGui::Begin("Viewport");

        ImVec2 cursor = ImGui::GetCursorPos();

        ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportOffset = ImGui::GetWindowPos();
        m_Description.Position = { viewportOffset.x, viewportOffset.y };
        m_Description.Bounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
        m_Description.Bounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

        m_Description.IsFocused = ImGui::IsWindowFocused();
        m_Description.IsHovered = ImGui::IsWindowHovered();

        const auto& [viewportX, viewportY] = ImGui::GetContentRegionAvail();
        m_Description.Size = { viewportX, viewportY };

        Ref<Image> image = m_ViewportRenderer->GetFinalImage();
        ImGui::Image(UI::GetTextureID(image), ImVec2((float)m_Description.Size.x, (float)m_Description.Size.y));

        if (ImGui::BeginDragDropTarget())
        {
            m_DragDropCallback();

            ImGui::EndDragDropTarget();
        }

        if (m_UIOverlayCallback)
        {
            ImGui::SetCursorPos(cursor);
            m_UIOverlayCallback();
        }

        if (m_ImGuizmoLayer)
        {
            m_ImGuizmoLayer->OnImGuiRender();
        }

        ImGui::End();
        ImGui::PopStyleVar();
	}

    void ViewportPanel::SetImGuizmoLayer(const Ref<ImGuizmoLayer>& layer)
    {
        m_ImGuizmoLayer = layer;
        m_ImGuizmoLayer->m_ViewportPanel = this;
    }
}
