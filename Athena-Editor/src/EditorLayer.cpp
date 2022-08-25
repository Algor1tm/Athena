#include "EditorLayer.h"

#include "Athena/Core/Input.h"
#include "Athena/Core/Application.h"
#include "Athena/Core/PlatformUtils.h"

#include "Athena/Debug/Instrumentor.h"
#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/RenderCommand.h"

#include "Athena/Scene/Components.h"
#include "Athena/Scene/SceneSerializer.h"

#include <ImGui/imgui.h>


namespace Athena
{
    EditorLayer::EditorLayer()
        : Layer("SandBox2D"), m_EditorCamera(Math::Radians(30.f), 16.f / 9.f, 0.1f, 1000.f)
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
        auto CheckerBoard = Texture2D::Create("assets/textures/CheckerBoard.png");
        auto KomodoHype = Texture2D::Create("assets/textures/KomodoHype.png");

        m_CameraController.SetZoomLevel(1.f);

        Entity SquareEntity = m_ActiveScene->CreateEntity("Square");
        SquareEntity.AddComponent<SpriteComponent>(LinearColor::Green);
        SquareEntity.GetComponent<TransformComponent>().Translation += Vector3(-1.f, 0, 0);

        Entity Komodo = m_ActiveScene->CreateEntity("KomodoHype");
        Komodo.AddComponent<SpriteComponent>(KomodoHype);
        Komodo.GetComponent<TransformComponent>().Translation += Vector3(2.f, 2.f, 0);

        Entity CameraEntity = m_ActiveScene->CreateEntity("Camera");
        CameraEntity.AddComponent<CameraComponent>();
        CameraEntity.GetComponent<CameraComponent>().Camera.SetOrthographicSize(10.f);

        class CameraScript: public NativeScript
        {
        public:
            void OnUpdate(Time frameTime) override
            {
                Vector3& position = GetComponent<TransformComponent>().Translation;
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

        CameraEntity.AddComponent<NativeScriptComponent>().Bind<CameraScript>();
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
            m_EditorCamera.SetViewportSize(m_ViewportSize.x, m_ViewportSize.y);
                
            m_ActiveScene->OnViewportResize(m_ViewportSize.x, m_ViewportSize.y);
        }

        if(m_ViewportHovered && !ImGuizmo::IsUsing())
            m_EditorCamera.OnUpdate(frameTime);

        Renderer2D::ResetStats();
        m_Framebuffer->Bind();
        RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });

        m_ActiveScene->OnUpdateEditor(frameTime, m_EditorCamera);

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
                if (ImGui::MenuItem("New", "Ctrl+N", false))
                {
                    NewScene();
                }

                if (ImGui::MenuItem("Open...", "Ctrl+O", false))
                {
                    OpenScene();
                }

                if (ImGui::MenuItem("Save As...", "Ctrl+S", false))
                {
                    SaveSceneAs();
                }

                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::MenuItem("Close", "Escape", false))
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
        Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered && !m_ViewportFocused);

        auto& [viewportX, viewportY] = ImGui::GetContentRegionAvail();
        m_ViewportSize = { viewportX, viewportY };

        uint32 texID = m_Framebuffer->GetColorAttachmentRendererID();
        ImGui::Image((void*)(uint64)texID, ImVec2((float)m_ViewportSize.x, (float)m_ViewportSize.y), { 0, 1 }, { 1, 0 });

        m_SelectedEntity = m_HierarchyPanel.GetSelectedEntity();
        //Gizmos
        if (m_SelectedEntity && m_GuizmoOperation != ImGuizmo::OPERATION::UNIVERSAL && m_SelectedEntity.HasComponent<TransformComponent>())
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, 
                ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
            
            //auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
            //const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
            //const Matrix4& cameraProjection = camera.GetProjection();
            //Matrix4 cameraView = Math::AffineInverse(cameraEntity.GetComponent<TransformComponent>().AsMatrix());
            const Matrix4& cameraProjection = m_EditorCamera.GetProjection();
            Matrix4 cameraView = m_EditorCamera.GetViewMatrix();

            auto& tc = m_SelectedEntity.GetComponent<TransformComponent>();
            Matrix4 transform = tc.AsMatrix();

            bool snap = Input::IsKeyPressed(Key::LCtrl);
            float snapValue = 0.5f;
            if (m_GuizmoOperation == ImGuizmo::OPERATION::ROTATE)
                snapValue = 45.f;

            Vector3 snapValues = Vector3(snapValue);

            ImGuizmo::Manipulate(cameraView.Data(), cameraProjection.Data(), 
                m_GuizmoOperation, ImGuizmo::LOCAL, transform.Data(), 
                nullptr, snap ? snapValues.Data() : nullptr);

            if (ImGuizmo::IsUsing())
            {
                Vector3 translation, rotation, scale;
                Math::DecomposeTransform(transform, translation, rotation, scale);

                tc.Translation = translation;
                tc.Rotation = rotation;
                tc.Scale = scale;
            }
        }

        ImGui::End();
        ImGui::PopStyleVar();
        
        ImGui::End();
    }

    void EditorLayer::OnEvent(Event& event)
    {
        m_EditorCamera.OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(ATN_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
    {
        if (event.GetRepeatCount() > 0)
            return false;

        bool ctrl = Input::IsKeyPressed(Key::LCtrl) || Input::IsKeyPressed(Key::RCtrl);

        switch (event.GetKeyCode())
        {
        case Key::S: if(ctrl) SaveSceneAs(); break;
        case Key::N: if(ctrl) NewScene(); break;
        case Key::O: if(ctrl) OpenScene(); break;
        //Gizmos
        case Key::Q: if(m_SelectedEntity)(m_GuizmoOperation = ImGuizmo::OPERATION::UNIVERSAL); break;
        case Key::W: if(m_SelectedEntity)(m_GuizmoOperation = ImGuizmo::OPERATION::TRANSLATE); break;
        case Key::E: if(m_SelectedEntity)(m_GuizmoOperation = ImGuizmo::OPERATION::ROTATE); break;
        case Key::R: if(m_SelectedEntity)(m_GuizmoOperation = ImGuizmo::OPERATION::SCALE); break;
        }

        return true;
    }


    void EditorLayer::NewScene()
    {
        m_ActiveScene = CreateRef<Scene>();
        m_ActiveScene->OnViewportResize(m_ViewportSize.x, m_ViewportSize.y);
        m_HierarchyPanel.SetContext(m_ActiveScene);
        ATN_CORE_INFO("Successfully created new scene");
    }

    void EditorLayer::SaveSceneAs()
    {
        String filepath = FileDialogs::SaveFile("Athena Scene (*atn)\0*.atn\0");
        if (!filepath.empty())
        {
            SceneSerializer serializer(m_ActiveScene);
            serializer.SerializeToFile(filepath.data());
            ATN_CORE_INFO("Successfully saved into '{0}'", filepath.data());
        }
    }

    void EditorLayer::OpenScene()
    {
        String filepath = FileDialogs::OpenFile("Athena Scene (*atn)\0*.atn\0");
        if (!filepath.empty())
        {
            m_ActiveScene = CreateRef<Scene>();
            m_ActiveScene->OnViewportResize(m_ViewportSize.x, m_ViewportSize.y);
            m_HierarchyPanel.SetContext(m_ActiveScene);

            SceneSerializer serializer(m_ActiveScene);
            serializer.DeserializeFromFile(filepath.data());
            ATN_CORE_INFO("Successfully loaded from '{0}'", filepath.data());
        }
    }
}
