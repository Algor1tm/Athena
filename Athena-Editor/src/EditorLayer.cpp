#include "EditorLayer.h"

#include "Athena/Core/Input.h"
#include "Athena/Core/Application.h"
#include "Athena/Core/PlatformUtils.h"

#include "Athena/Debug/Instrumentor.h"
#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/RenderCommand.h"

#include "Athena/Scene/Components.h"
#include "Athena/Scene/SceneSerializer.h"

#include "UI/Controllers.h"

#include <ImGui/imgui.h>


namespace Athena
{
    EditorLayer::EditorLayer()
        : Layer("SandBox2D"), m_EditorCamera(Math::Radians(30.f), 16.f / 9.f, 0.1f, 1000.f),
        m_PlayIcon("Resources/Icons/PlayIcon.png"), m_StopIcon("Resources/Icons/StopIcon.png"),
        m_SimulationIcon("Resources/Icons/SimulationIcon.png"), m_SaveEditorScenePath("Resources/tmp/EditorScene.atn")
    {

    }

    void EditorLayer::OnAttach()
    {
        ATN_PROFILE_FUNCTION();

        FramebufferDESC fbDesc;
        fbDesc.Attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
        fbDesc.Width = 1280;
        fbDesc.Height = 720;
        m_Framebuffer = Framebuffer::Create(fbDesc);
        m_ViewportSize = { fbDesc.Width, fbDesc.Height };

        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;
#if 0
        auto CheckerBoard = Texture2D::Create("assets/textures/CheckerBoard.png");
        auto KomodoHype = Texture2D::Create("assets/textures/KomodoHype.png");

        Entity SquareEntity = m_EditorScene->CreateEntity("Square");
        SquareEntity.AddComponent<SpriteComponent>(LinearColor::Green);
        SquareEntity.GetComponent<TransformComponent>().Translation += Vector3(-1.f, 0, 0);

        Entity Komodo = m_EditorScene->CreateEntity("KomodoHype");
        Komodo.AddComponent<SpriteComponent>(KomodoHype);
        Komodo.GetComponent<TransformComponent>().Translation += Vector3(1.f, 1.f, 0);

        Entity CameraEntity = m_EditorScene->CreateEntity("Camera");
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

#else
        m_EditorCamera.SetDistance(7.f);
        m_EditorCamera.Pan({ 0, 0.7f, 0 });
        OpenScene("assets/scene/PhysicsExample.atn");
#endif

        m_HierarchyPanel.SetContext(m_EditorScene);
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

        Renderer2D::ResetStats();
        m_Framebuffer->Bind();
        RenderCommand::Clear({ 0.1f, 0.1f, 0.1f, 1 });
        // Clear our entity ID attachment to -1
        m_Framebuffer->ClearAttachment(1, -1);

        switch (m_SceneState)
        {
        case SceneState::Edit:
        {
            if (m_ViewportHovered && !ImGuizmo::IsUsing())
                m_EditorCamera.OnUpdate(frameTime);

            m_ActiveScene->OnUpdateEditor(frameTime, m_EditorCamera); 
            break;

        }
        case SceneState::Play:
        {
            m_ActiveScene->OnUpdateRuntime(frameTime); 
            break;
        }
        case SceneState::Simulation:
        {
            if (m_ViewportHovered && !ImGuizmo::IsUsing())
                m_EditorCamera.OnUpdate(frameTime);

            m_ActiveScene->OnUpdateSimulation(frameTime, m_EditorCamera);
            break;
        }

        }

        RenderOverlay();

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
        if(m_ContentBrowserRendering)
            m_ContentBrowserPanel.OnImGuiRender();

        ImGui::Begin("Renderer2D Stats");

        auto stats = Renderer2D::GetStats();
        ImGui::Text("Draw Calls: %d", stats.DrawCalls);
        ImGui::Text("Quads: %d", stats.QuadCount);
        ImGui::Text("Circles: %d", stats.CircleCount);
        ImGui::Text("Lines: %d", stats.LineCount);
        ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
        ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

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

        ImGui::Begin("Editor Settings");

        UI::DrawController("Show Physics Colliders", 0, [this]() { return ImGui::Checkbox("##Show Physics Colliders", &m_ShowColliders); });

        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
        ImGui::Begin("Viewport");

        ImVec2 viewportMinRegion = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        ImVec2 viewportOffset = ImGui::GetWindowPos();
        m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
        m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

        m_ViewportFocused = ImGui::IsWindowFocused();
        m_ViewportHovered = ImGui::IsWindowHovered();    
        //Application::Get().GetImGuiLayer()->BlockEvents(!m_ViewportHovered && !m_ViewportFocused);
        Application::Get().GetImGuiLayer()->BlockEvents(false);

        auto& [viewportX, viewportY] = ImGui::GetContentRegionAvail();
        m_ViewportSize = { viewportX, viewportY };

        uint32 texID = m_Framebuffer->GetColorAttachmentRendererID(0);
        ImGui::Image((void*)(uint64)texID, ImVec2((float)m_ViewportSize.x, (float)m_ViewportSize.y), { 0, 1 }, { 1, 0 });

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
            {
                std::string_view path = (const char*)payload->Data;
                std::string_view extent = path.substr(path.size() - 4, path.size());
                //Scene Drag/Drop
                if (extent == ".atn\0")
                {
                    OpenScene(path.data());
                }
                //Texture Drag/Drop on Entity
                else if (m_SceneState == SceneState::Edit && extent == ".png\0")
                {
                    Entity target = GetEntityByCurrentMousePosition();
                    if (target.HasComponent<SpriteComponent>())
                    {
                        auto& sprite = target.GetComponent<SpriteComponent>();
                        sprite.Texture = Texture2D::Create(String(path));
                        sprite.Color = LinearColor::White;
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }


        m_SelectedEntity = m_HierarchyPanel.GetSelectedEntity();
        //Gizmos
        if (m_SceneState == SceneState::Edit && m_SelectedEntity && m_GuizmoOperation != ImGuizmo::OPERATION::BOUNDS && m_SelectedEntity.HasComponent<TransformComponent>())
        {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y,
                m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y - m_ViewportBounds[0].y);
            
            //auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
            //const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
            //const Matrix4& cameraProjection = camera.GetProjection();
            //Matrix4 cameraView = Math::AffineInverse(cameraEntity.GetComponent<TransformComponent>().AsMatrix());
            const Matrix4& cameraProjection = m_EditorCamera.GetProjection();
            const Matrix4& cameraView = m_EditorCamera.GetViewMatrix();

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

        Toolbar();

        ImGui::End();
    }

    void EditorLayer::Toolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(53.f / 255.f, 51.f / 255.f, 51.f / 255.f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(46.f / 255.f, 44.f / 255.f, 44.f / 255.f, 1.0f));

        ImGui::Begin("##Toolbar", nullptr, 
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        float size = ImGui::GetWindowHeight() - 8.f;
        {
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x * 0.5f - (size * 0.5f));
            void* iconID = m_SceneState != SceneState::Play ? m_PlayIcon.GetRendererIDvoid() : m_StopIcon.GetRendererIDvoid();
            if (ImGui::ImageButton(iconID, { size, size }))
            {
                if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulation)
                    OnScenePlay();
                else if (m_SceneState == SceneState::Play)
                    OnSceneStop();
            }
        }
        ImGui::SameLine();
        {
            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x * 0.5f - (size * 0.5f) + size * 1.25f);
            void* iconID = m_SceneState != SceneState::Simulation ? m_SimulationIcon.GetRendererIDvoid() : m_StopIcon.GetRendererIDvoid();
            if (ImGui::ImageButton(iconID, { size, size }))
            {
                if (m_SceneState == SceneState::Edit)
                    OnSceneSimulate();
                else if (m_SceneState == SceneState::Simulation)
                    OnSceneStop();
            }
        }

        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
    }

    Entity EditorLayer::GetEntityByCurrentMousePosition()
    {
        auto [mx, my] = ImGui::GetMousePos();
        mx -= m_ViewportBounds[0].x;
        my -= m_ViewportBounds[0].y;
        Vector2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
        my = viewportSize.y - my;

        int mouseX = (int)mx;
        int mouseY = (int)my;

        if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
        {
            m_Framebuffer->Bind();
            int pixelData = m_Framebuffer->ReadPixel(1, mouseX, mouseY);
            if (pixelData != -1)
                return { (entt::entity)pixelData, m_EditorScene.get() };
            m_Framebuffer->UnBind();
        }

        return Entity{};
    }

    void EditorLayer::RenderOverlay()
    {
        if (m_ShowColliders)
        {
            float offset = 0;

            if (m_SceneState == SceneState::Play)
            {
                Entity camera = m_ActiveScene->GetPrimaryCameraEntity();
                auto& runtimeCamera = camera.GetComponent<CameraComponent>().Camera;

                offset = camera.GetComponent<TransformComponent>().Translation.z >= 0 ? 0.001f : -0.001f;

                Renderer2D::BeginScene(runtimeCamera, camera.GetComponent<TransformComponent>().AsMatrix());
            }
            else if (m_SceneState == SceneState::Edit)
            {
                offset = m_EditorCamera.GetPosition().z >= 0 ? 0.001f : -0.001f;

                Renderer2D::BeginScene(m_EditorCamera);
            }

            {
                auto view = m_EditorScene->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
                for (auto entity : view)
                {
                    auto& [tc, bc2d] = view.get<TransformComponent, BoxCollider2DComponent>(entity);

                    Vector2 translation = Vector2(tc.Translation);
                    Vector2 scale = Vector2(tc.Scale) * bc2d.Size * 2.f;
                    Matrix4 transform = Math::ScaleMatrix(Vector3(scale)).Rotate(tc.Rotation.z, Vector3::back()).Translate(Vector3(translation, offset));

                    Renderer2D::DrawRect(transform, LinearColor::Green);
                }
            }

            {
                auto view = m_EditorScene->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
                for (auto entity : view)
                {
                    auto& [tc, cc2d] = view.get<TransformComponent, CircleCollider2DComponent>(entity);

                    Vector2 translation = Vector2(tc.Translation) + cc2d.Offset;
                    Vector2 scale = Vector2(tc.Scale) * Vector2(cc2d.Radius * 2);
                    Matrix4 transform = Math::ScaleMatrix(Vector3(scale)).Translate(Vector3(translation, offset));

                    Renderer2D::DrawCircle(transform, LinearColor::Green, 0.05f * 0.5f / cc2d.Radius);
                }
            }
        }


        if (m_SelectedEntity)
        {
            const TransformComponent& transform = m_SelectedEntity.GetComponent<TransformComponent>();
            Renderer2D::DrawRect(transform.AsMatrix(), { 1.f, 0.5f, 0.f, 1.f });
        }

        Renderer2D::EndScene();
    }

    void EditorLayer::OnScenePlay()
    {
        if (m_SceneState == SceneState::Simulation)
            OnSceneStop();

        m_HierarchyPanel.SetSelectedEntity(Entity{});

        SaveSceneAs(m_SaveEditorScenePath);
        m_SceneState = SceneState::Play;

        m_RuntimeScene = m_EditorScene;
        m_RuntimeScene->OnViewportResize(m_ViewportSize.x, m_ViewportSize.y);
        m_RuntimeScene->OnRuntimeStart();
        m_HierarchyPanel.SetContext(m_RuntimeScene);

        m_ActiveScene = m_RuntimeScene;
    }

    void EditorLayer::OnSceneStop()
    {
        m_SceneState = SceneState::Edit;

        m_ActiveScene = m_EditorScene;
        OpenScene(m_SaveEditorScenePath);
        m_HierarchyPanel.SetContext(m_EditorScene);

        m_RuntimeScene = nullptr;
    }

    void EditorLayer::OnSceneSimulate()
    {
        m_HierarchyPanel.SetSelectedEntity(Entity{});

        SaveSceneAs(m_SaveEditorScenePath);
        m_SceneState = SceneState::Simulation;

        m_RuntimeScene = m_EditorScene;
        m_RuntimeScene->OnViewportResize(m_ViewportSize.x, m_ViewportSize.y);
        m_RuntimeScene->OnSimulationStart();
        m_HierarchyPanel.SetContext(m_RuntimeScene);

        m_ActiveScene = m_RuntimeScene;
    }

    void EditorLayer::OnEvent(Event& event)
    {
        if(m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulation && m_ViewportHovered && !ImGuizmo::IsUsing())
            m_EditorCamera.OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(ATN_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonReleasedEvent>(ATN_BIND_EVENT_FN(EditorLayer::OnMouseReleased));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
    {
        if (event.IsRepeat())
            return false;

        bool ctrl = Input::IsKeyPressed(Key::LCtrl) || Input::IsKeyPressed(Key::RCtrl);

        switch (event.GetKeyCode())
        {
        // Scenes Management
        case Key::S: if(ctrl) SaveSceneAs(); break;
        case Key::N: if(ctrl) NewScene(); break;
        case Key::O: if(ctrl) OpenScene(); break;

        case Key::F4: if (m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulation) OnSceneStop(); break;
        case Key::F5: if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulation) OnScenePlay(); break;
        case Key::F6: if (m_SceneState == SceneState::Edit) OnSceneSimulate(); break;

        //Panels
        case Key::Space: if (ctrl) m_ContentBrowserRendering = !m_ContentBrowserRendering; break;

        //Gizmos
        case Key::Q: if(m_SelectedEntity && m_ViewportFocused)(m_GuizmoOperation = ImGuizmo::OPERATION::BOUNDS); break;
        case Key::W: if(m_SelectedEntity && m_ViewportFocused)(m_GuizmoOperation = ImGuizmo::OPERATION::TRANSLATE); break;
        case Key::E: if(m_SelectedEntity && m_ViewportFocused)(m_GuizmoOperation = ImGuizmo::OPERATION::ROTATE); break;
        case Key::R: if(m_SelectedEntity && m_ViewportFocused)(m_GuizmoOperation = ImGuizmo::OPERATION::SCALE); break;

        //Entities
        case Key::Escape: if (m_SelectedEntity) m_HierarchyPanel.SetSelectedEntity(Entity{}); break;
        case Key::Delete: 
        {
            if (m_SelectedEntity && m_SceneState == SceneState::Edit)
            {
                m_HierarchyPanel.SetSelectedEntity(Entity{});
                m_EditorScene->DestroyEntity(m_SelectedEntity); break;
            }
        }
        }

        return false;
    }

    bool EditorLayer::OnMouseReleased(MouseButtonReleasedEvent& event) 
    {
        if (ImGuizmo::IsOver())
            return false;

        switch(event.GetMouseButton())
        {
        case Mouse::Left:
        {
            Entity selectedEntity = GetEntityByCurrentMousePosition();
            if (selectedEntity)
                m_HierarchyPanel.SetSelectedEntity(selectedEntity);
            break;
        }
        }
        return false;
    }

    void EditorLayer::NewScene()
    {
        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;
        m_HierarchyPanel.SetContext(m_ActiveScene);
        ATN_CORE_INFO("Successfully created new scene");
    }

    void EditorLayer::SaveSceneAs()
    {
        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        String filepath = FileDialogs::SaveFile("Athena Scene (*atn)\0*.atn\0");
        if (!filepath.empty())
            SaveSceneAs(filepath);
        else
            ATN_CORE_ERROR("Invalid filepath to save Scene '{0}'", filepath.data());
    }

    void EditorLayer::SaveSceneAs(const std::filesystem::path& path)
    {
        SceneSerializer serializer(m_ActiveScene);
        serializer.SerializeToFile(path.string());
        ATN_CORE_INFO("Successfully saved Scene into '{0}'", path.string().data());
    }

    void EditorLayer::OpenScene()
    {
        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        String filepath = FileDialogs::OpenFile("Athena Scene (*atn)\0*.atn\0");
        if (!filepath.empty())
            OpenScene(filepath);
        else
            ATN_CORE_ERROR("Invalid filepath to load Scene '{0}'", filepath.data());
    }

    void EditorLayer::OpenScene(const std::filesystem::path& path)
    {
        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        Ref<Scene> newScene = CreateRef<Scene>();

        SceneSerializer serializer(newScene);
        if(serializer.DeserializeFromFile(path.string()))
        {
            m_EditorScene = newScene;
            m_ActiveScene = newScene;

            m_HierarchyPanel.SetContext(m_ActiveScene);
            ATN_CORE_INFO("Successfully load Scene from '{0}'", path.string().data());
        }
        else
        {
            ATN_CORE_ERROR("Failed to load Scene from '{0}'", path.string().data());
        }
    }
}
