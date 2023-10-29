#include "ProfilingPanel.h"

#include "Athena/Core/Application.h"

#include "Athena/UI/UI.h"

#include <ImGui/imgui.h>


namespace Athena
{
    ProfilingPanel::ProfilingPanel(std::string_view name, const Ref<EditorContext>& context)
        : Panel(name, context)
    {

    }

	void ProfilingPanel::OnImGuiRender()
	{
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8, 3 });
        if (ImGui::Begin("Profiling"))
        {
            bool vsync = Application::Get().GetWindow().IsVSync();
            bool newVsync = vsync;
            ImGui::Checkbox("VSync", &newVsync);
            if (newVsync != vsync)
                Application::Get().GetWindow().SetVSync(newVsync);

            const auto& appstats = Application::Get().GetStats();

            ImGui::Text("FPS: %d", (int)(1.f / appstats.FrameTime.AsSeconds()));
            ImGui::Text("FrameTime: %.3f ms", appstats.FrameTime.AsMilliseconds());
            ImGui::Spacing();

            ImGuiTabBarFlags tabBarFlags = 0;
            if (ImGui::BeginTabBar("ProfilingTabBar", tabBarFlags))
            {
                if (ImGui::BeginTabItem("Application"))
                {
                    uint32 memoryUsage = Platform::GetMemoryUsage() / (1024 * 1024);
                    ImGui::Text("RAM: %d Mb", memoryUsage);
                    ImGui::Text("VRAM: %d Mb", 0);
                    ImGui::Spacing();

                    ImGui::Text("Application::ProcessEvents: %.3f ms", appstats.Application_ProcessEvents.AsMilliseconds());
                    ImGui::Text("Application::OnUpdate: %.3f ms", appstats.Application_OnUpdate.AsMilliseconds());
                    ImGui::Text("Application::RenderImGui: %.3f ms", appstats.Application_RenderImGui.AsMilliseconds());
                    ImGui::Spacing();

                    ImGui::Text("Renderer::WaitAndRender: %.3f ms", appstats.Renderer_WaitAndRender.AsMilliseconds());
                    ImGui::Text("SwapChain::Present: %.3f ms", appstats.SwapChain_Present.AsMilliseconds());
                    ImGui::Text("SwapChain::AcquireImage: %.3f ms", appstats.SwapChain_AcquireImage.AsMilliseconds());
                    ImGui::Text("Renderer::QueueSubmit: %.3f ms", appstats.Renderer_QueueSubmit.AsMilliseconds());

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("SceneRenderer"))
                {

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("System Info"))
                {
                    if (UI::TreeNode("CPU Capabilites"))
                    {
                        auto& cpuCaps = Platform::GetCPUCapabilities();

                        ImGui::Text(cpuCaps.Name.c_str());
                        ImGui::Text("Processor Cores: %d", cpuCaps.NumberOfProcessorCores);
                        ImGui::Text("Logical Processors: %d", cpuCaps.NumberOfLogicalProcessors);
                        ImGui::Text("RAM: %d MB", cpuCaps.TotalPhysicalMemoryKB / 1024);

                        UI::TreePop();
                    }

                    if (UI::TreeNode("GPU Capabilites"))
                    {
                        UI::TreePop();
                    }

                    ImGui::EndTabItem();
                }
            }

            ImGui::EndTabBar();
        }
        ImGui::End();
        ImGui::PopStyleVar();
	}
}
