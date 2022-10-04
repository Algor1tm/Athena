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

        ImGui::Begin("Profiling");

        UI::DrawImGuiWidget("Renderer2D", [this]() { return ImGui::Checkbox("##Renderer2D", &m_IsShowRenderer2D); });
        if (m_IsShowRenderer2D)
        {
            auto stats = Renderer2D::GetStats();
            ImGui::BulletText("Draw Calls: %d", stats.DrawCalls);
            ImGui::BulletText("Quads: %d", stats.QuadCount);
            ImGui::BulletText("Circles: %d", stats.CircleCount);
            ImGui::BulletText("Lines: %d", stats.LineCount);
            ImGui::BulletText("Vertices: %d", stats.GetTotalVertexCount());
            ImGui::BulletText("Indices: %d", stats.GetTotalIndexCount());
        }


        ImGui::Separator();
        ImGui::Spacing();

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
        ImGui::Text("FPS: %d", (int)fps);
        ImGui::Text("FrameTime: %.3f", frameTime);

        if (m_IsPlottingFrameRate)
        {
            m_FrameRateStack[m_FrameRateIndex % m_FrameRateStack.size()] = m_FrameTime.AsSeconds();
            m_FrameRateIndex++;
        }

        UI::DrawImGuiWidget("Plot FrameRate", [this]() { return ImGui::Checkbox("##Plot FrameRate", &m_IsPlottingFrameRate); });
        if (m_IsPlottingFrameRate)
        {
            ImGui::PlotLines("##FrameRate", m_FrameRateStack.data(), (int)m_FrameRateStack.size(), 0, (const char*)0, 0, 0.1f, { ImGui::GetWindowSize().x * 0.9f, ImGui::GetWindowSize().y * 0.2f});
        }

        ImGui::End();
	}
}
