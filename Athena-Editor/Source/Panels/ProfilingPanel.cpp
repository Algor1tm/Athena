#include "ProfilingPanel.h"

#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Math/Trigonometric.h"

#include "UI/Widgets.h"

#include <ImGui/imgui.h>


namespace Athena
{
    ProfilingPanel::ProfilingPanel(std::string_view name)
        : Panel(name)
    {

    }

	void ProfilingPanel::OnImGuiRender()
	{
        Time now = m_Timer.ElapsedTime();
        m_FrameTime = now - m_LastTime;
        m_LastTime = now;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        ImGui::Begin("Profiling");
        ImGui::PopStyleVar();

        if(UI::BeginTreeNode("Renderer2D Statistics"))
        {
            auto stats = Renderer2D::GetStats();
            ImGui::Text("Draw Calls: %d", stats.DrawCalls);
            ImGui::Text("Quads: %d", stats.QuadCount);
            ImGui::Text("Circles: %d", stats.CircleCount);
            ImGui::Text("Lines: %d", stats.LineCount);
            ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
            ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

            UI::EndTreeNode();
        }


        static Time elapsed = Time(0);
        static float fps = 0.f;
        static float frameTime = 0.f;

        if (elapsed > m_UpdateInterval)
        {
            fps = 1 / m_FrameTime.AsSeconds();
            frameTime = m_FrameTime.AsMilliseconds();
            elapsed = Time(0);
        }
        else
        {
            elapsed += m_FrameTime;
        }

        if (UI::BeginTreeNode("Performance"))
        {
            ImGui::Text("FPS: %d", (int)fps);
            ImGui::Text("FrameTime: %.3f", frameTime);

            if (m_IsPlottingFrameRate)
            {
                m_FrameRateStack[m_FrameRateIndex % m_FrameRateStack.size()] = m_FrameTime.AsSeconds();
                m_FrameRateIndex++;
            }

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 3, 3 });
            UI::DrawImGuiWidget("Plot FrameRate", [this]() 
                {
                    return ImGui::Checkbox("##Plot FrameRate", &m_IsPlottingFrameRate); 
                });
            ImGui::PopStyleVar();
            if (m_IsPlottingFrameRate)
            {
                ImGui::PlotLines("##FrameRate", m_FrameRateStack.data(), (int)m_FrameRateStack.size(), 0, (const char*)0, 0, 0.1f, { ImGui::GetWindowSize().x * 0.9f, ImGui::GetWindowSize().y * 0.2f });
            }

            UI::EndTreeNode();
        }

        ImGui::End();
	}
}
