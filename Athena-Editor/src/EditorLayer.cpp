#include "EditorLayer.h"

#include <imgui.h>


static const uint32_t s_MapWidth = 24;
static const char s_MapTiles[] =
"WWWWWWWWWWWWWWWWWWWWWWWW"
"WWWWWWWWWWWWWWWWWWWWWWWW"
"WWWWWWWWWDDDDDWWWWWWWWWW"
"WWWWWDDDDDDDDDDDDDWWWWWW"
"WWWWDDDDDDDWWDDDDDDDWWWW"
"WWDDDDDDDDDDDDDDDDDDDWWW"
"WWDDDDDDDDDDDDDDDDDDDWWW"
"WWDDDDDDDDDDDDDDDDDDDWWW"
"WWDDDDDDDDDDDDDDDDDDDWWW"
"WWWDDDDWDDDDDDDDDDDDWWWW"
"WWWWWDDDDDDDDDDDDDWWWWWW"
"WWWWWWWWWDDDDDWWWWWWWWWW"
"WWWWWWWWWWWWWWWWWWWWWWWW";


namespace Athena
{
    EditorLayer::EditorLayer()
        : Layer("SandBox2D"), m_CameraController(16.f / 9.f, false), m_SquareColor(0.8f, 0.2f, 0.3f)
    {

    }

    void EditorLayer::OnAttach()
    {
        ATN_PROFILE_FUNCTION();

        FramebufferDesc fbDesc;
        fbDesc.Width = 1280;
        fbDesc.Height = 720;

        m_Framebuffer = Framebuffer::Create(fbDesc);
        
        m_ViewportSize = { fbDesc.Width, fbDesc.Height };

        m_CheckerBoard = Texture2D::Create("assets/textures/CheckerBoard.png");
        m_KomodoHype = Texture2D::Create("assets/textures/KomodoHype.png");
        m_SpriteSheet = Texture2D::Create("assets/game/textures/SpriteSheet.png");
        m_TextureMap['D'] = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 6, 11 }, { 128, 128 });
        m_TextureMap['W'] = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 11, 11 }, { 128, 128 });
        m_Barrel = SubTexture2D::CreateFromCoords(m_SpriteSheet, { 9, 0 }, { 128, 128 });

        m_CameraController.SetZoomLevel(5.f);
    }

    void EditorLayer::OnDetach()
    {
        ATN_PROFILE_FUNCTION();
    }

    void EditorLayer::OnUpdate(Time frameTime)
    {
        ATN_PROFILE_FUNCTION();

        if(m_ViewportHovered)
            m_CameraController.OnUpdate(frameTime);

        Renderer2D::ResetStats();
        {
            ATN_PROFILE_SCOPE("Renderer Clear");
            m_Framebuffer->Bind();
            RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });
        }

        {
#if 1
            static float rotation = 0.0f;
            rotation += frameTime.AsSeconds() * 1.f;

            ATN_PROFILE_SCOPE("Renderer Draw");
            Renderer2D::BeginScene(m_CameraController.GetCamera());

            Renderer2D::DrawQuad({ -1.f, 0.2f }, { 0.8f, 0.8f }, m_SquareColor);
            Renderer2D::DrawRotatedQuad({ 0.65f, 0.65f }, { 0.8f, 0.8f }, rotation, m_SquareColor);
            Renderer2D::DrawQuad({ 0.2f, -0.5f }, { 0.5f, 0.75f }, { 0.1f, 0.9f, 0.6f });
            Renderer2D::DrawQuad({ -0.f, -0.f, 0.1f }, { 10.f, 10.f }, m_CheckerBoard, 10, LinearColor(1.f, 0.95f, 0.95f));
            Renderer2D::DrawRotatedQuad({ -0.9f, -0.9f }, { 1.f, 1.f }, Radians(45), m_KomodoHype);

            Renderer2D::EndScene();
#else

            Renderer2D::BeginScene(m_CameraController.GetCamera());

            uint32_t mapHeight = (uint32_t)strlen(s_MapTiles) / s_MapWidth;
            for (uint32_t y = 0; y < mapHeight; ++y)
            {
                for (uint32_t x = 0; x < s_MapWidth; ++x)
                {
                    char tileType = s_MapTiles[x + y * s_MapWidth];
                    Ref<SubTexture2D> texture = m_Barrel;
                    if (m_TextureMap.find(tileType) != m_TextureMap.end())
                        texture = m_TextureMap[tileType];

                    Renderer2D::DrawQuad({ (float)x - (float)s_MapWidth / 2.f, (float)y - (float)mapHeight / 2.f }, { 1, 1 }, texture);
                }
            }

            Renderer2D::EndScene();
#endif

            m_Framebuffer->UnBind();
        }
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

        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Close", NULL, false))
                {
                    dockSpaceOpen = false;
                    Application::Get().Close();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::Begin("Renderer2D Stats");

        auto stats = Renderer2D::GetStats();
        ImGui::Text("Draw Calls: %d", stats.DrawCalls);
        ImGui::Text("Quads: %d", stats.QuadCount);
        ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
        ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

        ImGui::ColorEdit4("Square Color", m_SquareColor.Data());

        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
        ImGui::Begin("Viewport");

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();    
        Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered);

        ImVec2 tmpViewportSize = ImGui::GetContentRegionAvail();
        if (m_ViewportSize.x != tmpViewportSize.x || m_ViewportSize.y != tmpViewportSize.y)
        {
            m_ViewportSize = { tmpViewportSize.x, tmpViewportSize.y };
            m_Framebuffer->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

            m_CameraController.Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
        }
        else
        {
            uint32_t texID = m_Framebuffer->GetColorAttachmentRendererID();
            ImGui::Image((void*)(uint64_t)texID, ImVec2(m_ViewportSize.x, m_ViewportSize.y), { 0, 1 }, { 1, 0 });
        }

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::End();
    }

    void EditorLayer::OnEvent(Event& event)
    {
        m_CameraController.OnEvent(event);
    }
}
