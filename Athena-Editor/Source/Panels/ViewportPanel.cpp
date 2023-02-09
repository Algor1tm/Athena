#include "ViewportPanel.h"

#include "Athena/Core/Application.h"

#include "Athena/Renderer/Framebuffer.h"

#include "ImGuizmoLayer.h"

#include <ImGui/imgui.h>


namespace Athena
{
    ViewportPanel::ViewportPanel(std::string_view name)
        : Panel(name)
    {

    }

	void ViewportPanel::OnImGuiRender()
	{
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
        ImGui::Begin("Viewport");

        ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportOffset = ImGui::GetWindowPos();
        m_Description.Bounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
        m_Description.Bounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

        m_Description.IsFocused = ImGui::IsWindowFocused();
        m_Description.IsHovered = ImGui::IsWindowHovered();

        const auto& [viewportX, viewportY] = ImGui::GetContentRegionAvail();
        m_Description.Size = { viewportX, viewportY };

        void* texID = m_Description.AttachedFramebuffer->GetColorAttachmentRendererID(m_Description.AttachmentIndex);
        if (RendererAPI::GetAPI() == RendererAPI::OpenGL)        // TODO: make better
            ImGui::Image(texID, ImVec2((float)m_Description.Size.x, (float)m_Description.Size.y), { 0, 1 }, { 1, 0 });
        else
            ImGui::Image(texID, ImVec2((float)m_Description.Size.x, (float)m_Description.Size.y));

        if (ImGui::BeginDragDropTarget())
        {
            m_DragDropCallback();

            ImGui::EndDragDropTarget();
        }

        if (m_pImGuizmoLayer)
        {
            m_pImGuizmoLayer->OnImGuiRender();
        }

        ImGui::End();
        ImGui::PopStyleVar();
	}

    void ViewportPanel::SetImGuizmoLayer(class ImGuizmoLayer* layer)
    {
        m_pImGuizmoLayer = layer; 
        m_pImGuizmoLayer->m_pViewportPanel = this; 
    }
}
