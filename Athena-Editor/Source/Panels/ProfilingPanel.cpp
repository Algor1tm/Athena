#include "ProfilingPanel.h"

#include "Athena/Core/Application.h"

#include "Athena/UI/UI.h"

#include <ImGui/imgui.h>


namespace Athena
{
    ProfilingPanel::ProfilingPanel(std::string_view name, const Ref<EditorContext>& context)
        : Panel(name, context)
    {
        Platform::GetSystemInfo(&m_SystemInfo);
    }

	void ProfilingPanel::OnImGuiRender()
	{
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        if (ImGui::Begin("Profiling"))
        {
            if (UI::TreeNode("Platform"))
            {
                ImGui::Text("CPU: ");
				ImGui::Text(m_SystemInfo.CPUBrandString.c_str());
				ImGui::Text("Processor Cores: %d", m_SystemInfo.NumberOfProcessorCores);
				ImGui::Text("Logical Processors: %d", m_SystemInfo.NumberOfLogicalProcessors);
				ImGui::Text("RAM: %d MB", m_SystemInfo.TotalPhysicalMemoryKB / 1024);

				ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();

                //ImGui::Text("GPU: ");
				//ImGui::Text(m_GPUInfo.GPUBrandString.c_str());
				//ImGui::Text("Driver: %s", m_GPUInfo.APIVersion.c_str());
				//ImGui::Text("Vendor: %s", m_GPUInfo.Vendor.c_str());
				//ImGui::Text("VRAM: %d MB", m_GPUInfo.TotalPhysicalMemoryKB / 1024);

                UI::TreePop();
            }

            if (UI::TreeNode("Performance"))
            {
                auto appstats = Application::Get().GetStatistics();
                
                ImGui::Text("FPS: %d", (int)(1.f / appstats.FrameTime.AsSeconds()));
                ImGui::Text("FrameTime: %.3f", appstats.FrameTime.AsMilliseconds());
                ImGui::Text("Application::OnUpdate: %.3f", appstats.Application_OnUpdate.AsMilliseconds());
                ImGui::Text("Application::OnEvent: %.3f", appstats.Application_OnEvent.AsMilliseconds());
                ImGui::Text("Application::OnImGuiRender: %.3f", appstats.Application_OnImGuiRender.AsMilliseconds());
                ImGui::Text("Window::OnUpdate: %.3f", appstats.Window_SwapBuffers.AsMilliseconds());

                bool vsync = Application::Get().GetWindow().IsVSync();
                bool newVsync = vsync;
                ImGui::Checkbox("VSync", &newVsync);
                if (newVsync != vsync)
                    Application::Get().GetWindow().SetVSync(newVsync);

                UI::TreePop();
            }

        }

        ImGui::PopStyleVar();
        ImGui::End();
	}
}
