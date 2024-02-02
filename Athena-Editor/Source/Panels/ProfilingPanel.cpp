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
                    float memoryUsage = (float)Platform::GetMemoryUsage() / (1024.f * 1024.f);
                    float videoMemoryUsage = (float)Renderer::GetMemoryUsage() / (1024.f * 1024.f);
                    ImGui::Text("RAM: %.3f Mb", memoryUsage);
                    ImGui::Text("VRAM: %.3f Mb", videoMemoryUsage);
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
                    if (m_SceneRenderer)
                    {
                        size = m_SceneRenderer->GetViewportSize();
                        ImGui::Text("ViewportSize: { %u, %u }", size.x, size.y);

                        auto& stats = m_SceneRenderer->GetStatistics();
                        ImGui::Text("GeometryPass: %.3f ms", stats.GeometryPass.AsMilliseconds());
                        ImGui::Text("SceneCompositePass: %.3f ms", stats.SceneCompositePass.AsMilliseconds());
                        
                        if (UI::TreeNode("Pipeline Statistics"))
                        {
                            ImGui::Text("InputAssemblyVertices: %lld", stats.PipelineStats.InputAssemblyVertices);
                            ImGui::Text("InputAssemblyPrimitives: %lld", stats.PipelineStats.InputAssemblyPrimitives);
                            ImGui::Text("VertexShaderInvocations: %lld", stats.PipelineStats.VertexShaderInvocations);
                            ImGui::Text("GeometryShaderInvocations: %lld", stats.PipelineStats.GeometryShaderInvocations);
                            ImGui::Text("GeometryShaderPrimitives: %lld", stats.PipelineStats.GeometryShaderPrimitives);
                            ImGui::Text("ClippingInvocations: %lld", stats.PipelineStats.ClippingInvocations);
                            ImGui::Text("ClippingPrimitives: %lld", stats.PipelineStats.ClippingPrimitives);
                            ImGui::Text("FragmentShaderInvocations: %lld", stats.PipelineStats.FragmentShaderInvocations);
                            ImGui::Text("ComputeShaderInvocations: %lld", stats.PipelineStats.ComputeShaderInvocations);

                            UI::TreePop();
                        }
                    }

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
                        auto& gpuCaps = Renderer::GetRenderCaps();

                        ImGui::Text(gpuCaps.Name.c_str());
                        ImGui::Text("VRAM: %d Mb", gpuCaps.VRAM / 1024);

                        ImGui::Text("MaxImageDimension2D: %u", gpuCaps.MaxImageDimension2D);
                        ImGui::Text("MaxImageDimensionCube: %u", gpuCaps.MaxImageDimensionCube);
                        ImGui::Text("MaxImageArrayLayers: %u", gpuCaps.MaxImageArrayLayers);
                        ImGui::Spacing();
                        ImGui::Text("MaxSamplerLodBias: %.1f", gpuCaps.MaxSamplerLodBias);
                        ImGui::Text("MaxSamplerAnisotropy: %.1f", gpuCaps.MaxSamplerAnisotropy);
                        ImGui::Spacing();
                        ImGui::Text("MaxFramebufferWidth: %u", gpuCaps.MaxFramebufferWidth);
                        ImGui::Text("MaxFramebufferHeight: %u", gpuCaps.MaxFramebufferHeight);
                        ImGui::Text("MaxFramebufferLayers: %u", gpuCaps.MaxFramebufferLayers);
                        ImGui::Text("MaxFramebufferColorAttachments: %u", gpuCaps.MaxFramebufferColorAttachments);
                        ImGui::Spacing();
                        ImGui::Text("MaxUniformBufferRange: %u", gpuCaps.MaxUniformBufferRange);
                        ImGui::Text("MaxStorageBufferRange: %u", gpuCaps.MaxStorageBufferRange);
                        ImGui::Text("MaxPushConstantRange: %u", gpuCaps.MaxPushConstantRange);
                        ImGui::Spacing();
                        ImGui::Text("MaxBoundDescriptorSets: %u", gpuCaps.MaxBoundDescriptorSets);
                        ImGui::Text("MaxDescriptorSetSamplers: %u", gpuCaps.MaxDescriptorSetSamplers);
                        ImGui::Text("MaxDescriptorSetUniformBuffers: %u", gpuCaps.MaxDescriptorSetUnifromBuffers);
                        ImGui::Text("MaxDescriptorSetStorageBuffers: %u", gpuCaps.MaxDescriptorSetStorageBuffers);
                        ImGui::Text("MaxDescriptorSetSampledImages: %u", gpuCaps.MaxDescriptorSetSampledImages);
                        ImGui::Text("MaxDescriptorSetStorageImages: %u", gpuCaps.MaxDescriptorSetStorageImages);
                        ImGui::Text("MaxDescriptorSetInputAttachments: %u", gpuCaps.MaxDescriptorSetInputAttachments);
                        ImGui::Spacing();
                        ImGui::Text("MaxViewportDimensions: { %u, %u }", gpuCaps.MaxViewportDimensions[0], gpuCaps.MaxViewportDimensions[1]);
                        ImGui::Text("MaxClipDistances: %u", gpuCaps.MaxClipDistances);
                        ImGui::Text("MaxCullDistances: %u", gpuCaps.MaxCullDistances);
                        ImGui::Text("LineWidthRange: { %.1f, %.1f }", gpuCaps.LineWidthRange[0], gpuCaps.LineWidthRange[1]);
                        ImGui::Spacing();
                        ImGui::Text("MaxVertexInputAttributes: %u", gpuCaps.MaxVertexInputAttributes);
                        ImGui::Text("MaxVertexInputBindingStride: %u", gpuCaps.MaxVertexInputBindingStride);
                        ImGui::Text("MaxFragmentInputComponents: %u", gpuCaps.MaxFragmentInputComponents);
                        ImGui::Text("MaxFragmentOutputAttachments: %u", gpuCaps.MaxFragmentOutputAttachments);
                        ImGui::Spacing();
                        ImGui::Text("MaxComputeWorkGroupSize: { %u, %u, %u }", gpuCaps.MaxComputeWorkGroupSize[0], gpuCaps.MaxComputeWorkGroupSize[1], gpuCaps.MaxComputeWorkGroupSize[2]);
                        ImGui::Text("MaxComputeSharedMemorySize: %u", gpuCaps.MaxComputeSharedMemorySize);
                        ImGui::Text("MaxComputeWorkGroupInvocations: %u", gpuCaps.MaxComputeWorkGroupInvocations);
                        ImGui::Spacing();
                        ImGui::Text("TimestampComputeAndGraphics: %s", gpuCaps.TimestampComputeAndGraphics ? "true" : "false");
                        ImGui::Text("TimestampPeriod: %f", gpuCaps.TimestampPeriod);

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
