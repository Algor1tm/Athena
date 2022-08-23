#include "EditorLayer.h"

#include "Athena/Core/Input.h"
#include "Athena/Core/Application.h"
#include "Athena/Debug/Instrumentor.h"
#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/RenderCommand.h"
#include "Athena/Scene/Components.h"
#include "Athena/Scene/SceneSerializer.h"

#include <ImGui/imgui.h>


namespace Athena
{
    EditorLayer::EditorLayer()
        : Layer("SandBox2D"), m_CameraController(16.f / 9.f, false)
    {

    }

    void EditorLayer::OnAttach()
    {
        ATN_PROFILE_FUNCTION();

        FramebufferDESC fbDesc;
        fbDesc.Width = 1280;
        fbDesc.Height = 720;
        m_Framebuffer = Framebuffer::Create(fbDesc);
        m_ViewportSize = { fbDesc.Width, fbDesc.Height };

        m_ActiveScene = CreateRef<Scene>();
#if 0
        m_CheckerBoard = Texture2D::Create("assets/textures/CheckerBoard.png");
        m_KomodoHype = Texture2D::Create("assets/textures/KomodoHype.png");

        m_CameraController.SetZoomLevel(1.f);

        m_SquareEntity = m_ActiveScene->CreateEntity("Square");
        m_SquareEntity.AddComponent<SpriteComponent>(LinearColor::Green);
        m_SquareEntity.GetComponent<TransformComponent>().Position += Vector3(-1.f, 0, 0);

        m_Komodo = m_ActiveScene->CreateEntity("KomodoHype");
        m_Komodo.AddComponent<SpriteComponent>(m_KomodoHype);
        m_Komodo.GetComponent<TransformComponent>().Position += Vector3(2.f, 2.f, 0);

        m_CameraEntity = m_ActiveScene->CreateEntity("Camera");
        m_CameraEntity.AddComponent<CameraComponent>();
        m_CameraEntity.GetComponent<CameraComponent>().Camera.SetOrthographicSize(10.f);

        class CameraScript: public NativeScript
        {
        public:
            void OnUpdate(Time frameTime) override
            {
                Vector3& position = GetComponent<TransformComponent>().Position;
                static float speed = 10.f;

                if (Input::IsKeyPressed(Key::A))
                    position.x -= speed * frameTime.AsSeconds();
                if (Input::IsKeyPressed(Key::D))
                    position.x += speed * frameTime.AsSeconds();
                if (Input::IsKeyPressed(Key::S))
                    position.y -= speed * frameTime.AsSeconds();
                if (Input::IsKeyPressed(Key::W))
                    position.y += speed * frameTime.AsSeconds();
            }
        };

        m_CameraEntity.AddComponent<NativeScriptComponent>().Bind<CameraScript>();
#endif

        m_HierarchyPanel.SetContext(m_ActiveScene);
    }

    void EditorLayer::OnDetach()
    {
        ATN_PROFILE_FUNCTION();
    }

    void EditorLayer::OnUpdate(Time frameTime)
    {
        ATN_PROFILE_FUNCTION();

        m_FrameTime = frameTime;

        const auto& desc = m_Framebuffer->GetDescription();
        if (m_ViewportSize.x > 0 && m_ViewportSize.y > 0 &&
            (desc.Width != m_ViewportSize.x || desc.Height != m_ViewportSize.y)) 
        {
            m_Framebuffer->Resize(m_ViewportSize.x, m_ViewportSize.y);
            m_CameraController.Resize(m_ViewportSize.x, m_ViewportSize.y);

            m_ActiveScene->OnViewportResize(m_ViewportSize.x, m_ViewportSize.y);
        }

        if(m_ViewportHovered)
            m_CameraController.OnUpdate(frameTime);

        Renderer2D::ResetStats();
        m_Framebuffer->Bind();
        RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });

        m_ActiveScene->OnUpdate(frameTime);

        m_Framebuffer->UnBind();
    }

    void EditorLayer::OnImGuiRender()
    {
        ATN_PROFILE_FUNCTION();

        static bool dockSpaceOpen = true;

        static bool opt_fullscreen = true;
        static bool opt_padding = false;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
        // because it would be confusing to have two docking targets within each others.
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        else
        {
            dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
        }

        // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
        // and handle the pass-thru hole, so we ask Begin() to not render a background.
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
        // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
        // all active windows docked into it will lose their parent and become undocked.
        // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
        // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
        if (!opt_padding)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", &dockSpaceOpen, window_flags);
        if (!opt_padding)
            ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        //DockSpace
        ImGuiIO& io = ImGui::GetIO();
        // Submit the DockSpace
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save", NULL, false))
                {
                    std::string_view savePath = "assets/scene/Example.atnscene";
                    SceneSerializer serializer(m_ActiveScene);
                    serializer.SerializeToFile(savePath.data());
                    ATN_CORE_INFO("Successfully saved into '{0}'", savePath.data());
                }

                if (ImGui::MenuItem("Load", NULL, false))
                {
                    std::string_view loadPath = "assets/scene/Example.atnscene";
                    SceneSerializer serializer(m_ActiveScene);
                    serializer.DeserializeFromFile(loadPath.data());
                    ATN_CORE_INFO("Successfully loaded from '{0}'", loadPath.data());
                }

                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::MenuItem("Close", NULL, false))
                {
                    dockSpaceOpen = false;
                    Application::Get().Close();
                }

                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        m_HierarchyPanel.OnImGuiRender();

        ImGui::Begin("Renderer2D Stats");

        auto stats = Renderer2D::GetStats();
        ImGui::Text("Draw Calls: %d", stats.DrawCalls);
        ImGui::Text("Quads: %d", stats.QuadCount);
        ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
        ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

        static Timer fpsUpdateTimer;
        static Time updateInterval = Time::Seconds(0.05f);
        static Time elapsed = Time(0);
        static float fps = 0.f;
        static float frameTime = 0.f;

        if (elapsed > updateInterval)
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

        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        ImGui::Begin("Viewport");

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();    
        Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);

        auto& [viewportX, viewportY] = ImGui::GetContentRegionAvail();
        m_ViewportSize = { viewportX, viewportY };

        uint32 texID = m_Framebuffer->GetColorAttachmentRendererID();
        ImGui::Image((void*)(uint64)texID, ImVec2((float)m_ViewportSize.x, (float)m_ViewportSize.y), { 0, 1 }, { 1, 0 });

        ImGui::End();
        ImGui::PopStyleVar();
        
        ImGui::End();
    }

    void EditorLayer::OnEvent(Event& event)
    {
        m_CameraController.OnEvent(event);
    }
}
