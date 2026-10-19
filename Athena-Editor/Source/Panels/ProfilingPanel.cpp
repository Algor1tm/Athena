#include "ProfilingPanel.h"

#include "Athena/Core/Stats.h"
#include "Athena/Core/Application.h"
#include "Athena/Renderer/RHI/RHI.h"
#include "Athena/UI/UI.h"
#include "Athena/Utils/StringUtils.h"
#include "Panels/PanelManager.h"

#include <ImGui/imgui.h>


namespace Athena
{
    ProfilingPanel::ProfilingPanel(const Ref<EditorContext>& context)
        : Panel(PROFILING_PANEL_ID, context)
    {

    }

	void ProfilingPanel::OnImGuiRender()
	{
        ENQUEUE_RENDER_COMMAND([this]() 
            {
                m_VRAMUsage = RHI::Get()->GetMemoryUsage();
            });

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8, 3 });
        if (ImGui::Begin("Profiling"))
        {
            bool vsync = Application::Get().GetWindow().IsVSync();
            if (ImGui::Checkbox("VSync", &vsync))
                Application::Get().GetWindow().SetVSync(vsync);

            Time frameTime = Application::Get().GetFrameTime();

            ImGui::Text("FPS: %d", (int)(1.f / frameTime.AsSeconds()));
            ImGui::Text("FrameTime: %.3f ms", frameTime.AsMilliseconds());
            
            Vector2u size;
            size.x = Application::Get().GetWindow().GetWidth();
            size.y = Application::Get().GetWindow().GetHeight();
            ImGui::Text("WindowSize: { %u, %u }", size.x, size.y);

            ImGui::Spacing();

            ImGuiTabBarFlags tabBarFlags = 0;
            if (ImGui::BeginTabBar("ProfilingTabBar", tabBarFlags))
            {
                if (ImGui::BeginTabItem("Application"))
                {
                    ImGui::Text("RAM: %s", Utils::MemoryBytesToString(Platform::GetMemoryUsage()).data());
                    ImGui::Text("VRAM: %s", Utils::MemoryBytesToString(m_VRAMUsage).data());
                    ImGui::Spacing();

                    const std::unordered_map<std::string_view, StatsGroup*>& statsGroups = StatsSystem::Get().GetAllStatsGroups();

                    for (const auto& [name, group] : statsGroups)
                    {
                        ImGui::Text(name.data());
                        ImGui::Separator();

                        StatsGroup::Snapshot snapshot = group->GetSnapshot();
                        for (const auto& cycleStat : snapshot.CycleStats)
                        {
                            ImGui::Text("%s: %.3f ms", cycleStat.Name.data(), cycleStat.Value.AsMilliseconds());
                        }

                        for (const auto& counterStat : snapshot.CounterStats)
                        {
                            ImGui::Text("%s: %d", counterStat.Name.data(), counterStat.Value);
                        }

                        ImGui::Spacing();
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("SceneRenderer"))
                {
                //    if (m_SceneRenderer)
                //    {
                //        auto& settings = m_SceneRenderer->GetSettings();
                //        //Antialising antialising = m_SceneRenderer->GetAntialising();
                //        size = m_SceneRenderer->GetViewportSize();

                //        ImGui::Text("ViewportSize: { %u, %u }", size.x, size.y);

                //        auto& stats = m_SceneRenderer->GetStatistics();
                //        ImGui::Text("GPUTime: %.3f ms", stats.GPUTime.AsMilliseconds());
                //        ImGui::Text("DirShadowMap: %.3f ms", stats.DirShadowMapPass.AsMilliseconds());
                //        ImGui::Text("GBuffer: %.3f ms", stats.GBufferPass.AsMilliseconds());
                //        ImGui::Text("HiZ: %.3f ms", stats.HiZPass.AsMilliseconds());
                //        ImGui::Text("LightCulling: %.3f ms", stats.LightCullingPass.AsMilliseconds());

                //        if (settings.AOSettings.Enable)
                //        {
                //            ImGui::Text("HBAO-Deinterleave: %.3f ms", stats.HBAODeinterleavePass.AsMilliseconds());
                //            ImGui::Text("HBAO-Compute: %.3f ms", stats.HBAOComputePass.AsMilliseconds());
                //            ImGui::Text("HBAO-Blur: %.3f ms", stats.HBAOBlurPass.AsMilliseconds());
                //        }

                //        ImGui::Text("DeferredLighting: %.3f ms", stats.DeferredLightingPass.AsMilliseconds());
                //        ImGui::Text("Skybox: %.3f ms", stats.SkyboxPass.AsMilliseconds());

                //        if (settings.SSRSettings.Enable)
                //        {
                //            ImGui::Text("Pre-Convolution: %.3f ms", stats.PreConvolutionPass.AsMilliseconds());
                //            ImGui::Text("SSR-Compute: %.3f ms", stats.SSRComputePass.AsMilliseconds());
                //            ImGui::Text("SSR-Composite: %.3f ms", stats.SSRCompositePass.AsMilliseconds());
                //        }

                //        if (settings.BloomSettings.Enable)
                //        {
                //            ImGui::Text("Bloom: %.3f ms", stats.BloomPass.AsMilliseconds());
                //        }

                //        ImGui::Text("SceneComposite: %.3f ms", stats.SceneCompositePass.AsMilliseconds());
                //        ImGui::Text("JumpFlood: %.3f ms", stats.JumpFloodPass.AsMilliseconds());
                //        ImGui::Text("Render2D: %.3f ms", stats.Render2DPass.AsMilliseconds());

                //        if (antialising == Antialising::FXAA)
                //            ImGui::Text("FXAA: %.3f ms", stats.AAPass.AsMilliseconds());
                //        else if (antialising == Antialising::SMAA)
                //            ImGui::Text("SMAA: %.3f ms", stats.AAPass.AsMilliseconds());

                //        if (UI::TreeNode("Pipeline Statistics", false))
                //        {
                //            ImGui::Text("InputAssemblyVertices: %lld", stats.PipelineStats.InputAssemblyVertices);
                //            ImGui::Text("InputAssemblyPrimitives: %lld", stats.PipelineStats.InputAssemblyPrimitives);
                //            ImGui::Text("VertexShaderInvocations: %lld", stats.PipelineStats.VertexShaderInvocations);
                //            ImGui::Text("GeometryShaderInvocations: %lld", stats.PipelineStats.GeometryShaderInvocations);
                //            ImGui::Text("GeometryShaderPrimitives: %lld", stats.PipelineStats.GeometryShaderPrimitives);
                //            ImGui::Text("ClippingInvocations: %lld", stats.PipelineStats.ClippingInvocations);
                //            ImGui::Text("ClippingPrimitives: %lld", stats.PipelineStats.ClippingPrimitives);
                //            ImGui::Text("FragmentShaderInvocations: %lld", stats.PipelineStats.FragmentShaderInvocations);
                //            ImGui::Text("ComputeShaderInvocations: %lld", stats.PipelineStats.ComputeShaderInvocations);

                //            UI::TreePop();
                //        }

                //        if (UI::TreeNode("Draw Statistics", false))
                //        {
                //            ImGui::Text("Meshes: %u", stats.Meshes);
                //            ImGui::Text("Instances: %u", stats.Instances);
                //            ImGui::Text("Draws saved(by instancing): %u", stats.Meshes - stats.Instances);
                //            ImGui::Spacing();
                //            ImGui::Text("AnimMeshes: %u", stats.AnimMeshes);

                //            UI::TreePop();
                //        }
                //    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("System Info"))
                {
                    if (UI::TreeNode("CPU Capabilites"))
                    {
                        auto& cpuCaps = Platform::GetCPUCapabilities();

                        ImGui::Text(cpuCaps.Name.c_str());
                        ImGui::Text("RAM: %d Mb", cpuCaps.RAM / 1024);
                        ImGui::Text("Cores: %d", cpuCaps.Cores);
                        ImGui::Text("Logical Processors: %d", cpuCaps.LogicalProcessors);

                        UI::TreePop();
                    }

                    if (UI::TreeNode("GPU Capabilites"))
                    {
                        auto& gpuCaps = RHI::Get()->GetGPUCapabilities();

                        ImGui::Text(gpuCaps.Name.c_str());
                        ImGui::Text("VRAM: %d Mb", gpuCaps.VRAM / 1024);

                        uint32 MaxTextureDimension2D;
                        uint32 MaxTextureDimensionCube;
                        uint32 MaxFramebufferWidth;
                        uint32 MaxFramebufferHeight;
                        uint32 MaxViewportWidth;
                        uint32 MaxViewportHeight;

                        ImGui::Text("MaxTextureDimension2D: %u", gpuCaps.MaxTextureDimension2D);
                        ImGui::Text("MaxTextureDimensionCube: %u", gpuCaps.MaxTextureDimensionCube);
                        ImGui::Text("MaxFramebufferWidth: %u", gpuCaps.MaxFramebufferWidth);
                        ImGui::Text("MaxFramebufferWidth: %u", gpuCaps.MaxFramebufferWidth);
                        ImGui::Text("MaxViewportWidth: %u", gpuCaps.MaxViewportWidth);
                        ImGui::Text("MaxViewportHeight: %u", gpuCaps.MaxViewportHeight);

                        ImGui::Spacing();

                        ImGui::Text("MaxSamplerLodBias: %.1f", gpuCaps.MaxSamplerLodBias);
                        ImGui::Text("MaxSamplerAnisotropy: %.1f", gpuCaps.MaxSamplerAnisotropy);

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
