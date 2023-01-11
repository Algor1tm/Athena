#include "EditorLayer.h"

#include "Athena/Input/Input.h"
#include "Athena/Core/Application.h"
#include "Athena/Core/PlatformUtils.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/RenderCommand.h"

#include "Athena/Scene/Components.h"
#include "Athena/Scene/SceneSerializer.h"

#include <ImGui/imgui.h>


namespace Athena
{
    EditorLayer::EditorLayer()
        : Layer("SandBox2D"), m_EditorCamera(Math::Radians(30.f), 16.f / 9.f, 0.1f, 1000.f),
        m_TemporaryEditorScenePath("Resources/Tmp/EditorScene.atn"), m_ImGuizmoLayer(&m_EditorCamera)
    {
        m_PlayIcon = Texture2D::Create("Resources/Icons/Editor/MenuBar/PlayIcon.png");
        m_SimulationIcon = Texture2D::Create("Resources/Icons/Editor/MenuBar/SimulationIcon.png");
        m_StopIcon = Texture2D::Create("Resources/Icons/Editor/MenuBar/StopIcon.png");
    }

    void EditorLayer::OnAttach()
    {
        Application::Get().GetImGuiLayer()->BlockEvents(false);
        InitializePanels();

        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;

        m_EditorCamera.SetDistance(7.f);
        m_EditorCamera.Pan({ 0, 0.7f, 0 });
        //OpenScene("Assets/Scenes/PhysicsExample.atn");

        uint32 indices[] = 
        {
            0, 1, 2, // Side 0
            2, 1, 3,
            4, 0, 6, // Side 1
            6, 0, 2,
            7, 5, 6, // Side 2
            6, 5, 4,
            3, 1, 7, // Side 3 
            7, 1, 5,
            4, 5, 0, // Side 4 
            0, 5, 1,
            3, 7, 2, // Side 5 
            2, 7, 6
        };

        BufferLayout layout =
        {
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float4, "a_Color" }
        };

        struct Vertex
        {
            Vector3 Position;
            LinearColor Color;
        };

        Vertex vertices[] =
        {
            { { -0.5f,  0.5f, -0.5f }, LinearColor::Cyan },
            { {  0.5f,  0.5f, -0.5f }, LinearColor::Red },
            { { -0.5f, -0.5f, -0.5f }, LinearColor::Yellow },
            { {  0.5f, -0.5f, -0.5f }, LinearColor::Blue },
            { { -0.5f,  0.5f,  0.5f }, LinearColor::Green },
            { {  0.5f,  0.5f,  0.5f }, LinearColor::White },
            { { -0.5f, -0.5f,  0.5f }, LinearColor::Magenta },
            { {  0.5f, -0.5f,  0.5f }, LinearColor::Transparent }
        };

        Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32));

        VertexBufferDescription vBufferDesc;
        vBufferDesc.Data = vertices;
        vBufferDesc.Size = sizeof(vertices);
        vBufferDesc.pBufferLayout = &layout;
        vBufferDesc.pIndexBuffer = indexBuffer;
        vBufferDesc.BufferUsage = Usage::STATIC;

        Ref<VertexBuffer> vb = VertexBuffer::Create(vBufferDesc);

        Ref<Mesh> mesh = Mesh::Create(vb);

        m_TestCube = m_ActiveScene->CreateEntity();
        m_TestCube.AddComponent<MeshComponent>();
        m_TestCube.GetComponent<MeshComponent>().Mesh = mesh;

        m_SceneHierarchy->SetContext(m_EditorScene);
    }

    void EditorLayer::OnDetach()
    {

    }

    void EditorLayer::OnUpdate(Time frameTime)
    {
        const auto& vpDesc = m_MainViewport->GetDescription();
        const auto& framebuffer = Renderer::GetFramebuffer();
        const auto& framebufferDesc = framebuffer->GetDescription();

        if (vpDesc.Size.x > 0 && vpDesc.Size.y > 0 &&
            (framebufferDesc.Width != vpDesc.Size.x || framebufferDesc.Height != vpDesc.Size.y))
        {
            framebuffer->Resize(vpDesc.Size.x, vpDesc.Size.y);
            m_EditorCamera.SetViewportSize(vpDesc.Size.x, vpDesc.Size.y);
                
            m_ActiveScene->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
        }

        Renderer2D::ResetStats();
        Renderer::Clear({ 0.1f, 0.1f, 0.1f, 1 });
        // Clear our entity ID attachment to -1
        framebuffer->ClearAttachment(1, -1);

        switch (m_SceneState)
        {
        case SceneState::Edit:
        {
            if (vpDesc.IsHovered && !ImGuizmo::IsUsing())
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
            if (vpDesc.IsHovered && !ImGuizmo::IsUsing())
                m_EditorCamera.OnUpdate(frameTime);

            m_ActiveScene->OnUpdateSimulation(frameTime, m_EditorCamera);
            break;
        }
        }

        RenderOverlay();

        RenderCommand::UnBindFramebuffer(); // TODO: Remove
    }

    void EditorLayer::OnImGuiRender()
    {
        static bool dockSpaceOpen = true;
        static constexpr ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::Begin("DockSpace Demo", &dockSpaceOpen, window_flags);
        ImGui::PopStyleVar(3);

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        }

        if(m_SceneState != SceneState::Play)
            SelectEntity(m_SceneHierarchy->GetSelectedEntity());

        m_PanelManager.OnImGuiRender();

        ImGui::End();
    }

    void EditorLayer::SelectEntity(Entity entity)
    {
        m_ImGuizmoLayer.SetActiveEntity(entity);
        m_SceneHierarchy->SetSelectedEntity(entity);
        m_SelectedEntity = entity;
    }

    void EditorLayer::InitializePanels()
    {
        m_MainMenuBar = CreateRef<MenuBarPanel>("MainMenuBar");
        m_MainMenuBar->SetLogoIcon(Texture2D::Create("Resources/Icons/Editor/MenuBar/Logo-no-background.png"));
        m_MainMenuBar->AddMenuItem("File", [this]()
            {
                if (ImGui::MenuItem("New", "Ctrl+N", false))
                    NewScene();

                if (ImGui::MenuItem("Open...", "Ctrl+O", false))
                    OpenScene();

                if (ImGui::MenuItem("Save As...", "Ctrl+S", false))
                    SaveSceneAs();

                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::MenuItem("Exit", NULL, false))
                    Application::Get().Close();
            });

        m_MainMenuBar->AddMenuItem("View", [this]()
            {
                m_PanelManager.ImGuiRenderAsMenuItems();
            });

        m_MainMenuBar->AddMenuButton(m_PlayIcon, [this](Ref<Texture2D>& currentIcon)
            {
                currentIcon = m_SceneState == SceneState::Edit ? m_StopIcon : m_PlayIcon;

                if (m_SceneState == SceneState::Edit)
                    OnScenePlay();
                else if (m_SceneState == SceneState::Play)
                    OnSceneStop();
            });

        m_MainMenuBar->AddMenuButton(m_SimulationIcon, [this](Ref<Texture2D>& currentIcon)
            {
                currentIcon = m_SceneState == SceneState::Edit ? m_StopIcon : m_SimulationIcon;

                if (m_SceneState == SceneState::Edit)
                    OnSceneSimulate();
                else if (m_SceneState == SceneState::Simulation)
                    OnSceneStop();
            });

        m_PanelManager.AddPanel(m_MainMenuBar, false);


        m_MainViewport = CreateRef<ViewportPanel>("MainViewport");

        m_MainViewport->SetImGuizmoLayer(&m_ImGuizmoLayer);
        m_MainViewport->SetFramebuffer(Renderer::GetFramebuffer(), 0);
        m_MainViewport->SetDragDropCallback([this]()
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    std::string_view path = (const char*)payload->Data;
                    std::string_view extension = path.substr(path.size() - 4, path.size());
                    //Scene Drag/Drop
                    if (extension == ".atn\0")
                    {
                        OpenScene(path.data());
                    }
                    //Texture Drag/Drop on Entity
                    else if (m_SceneState == SceneState::Edit && extension == ".png\0")
                    {
                        Entity target = GetEntityByCurrentMousePosition();
                        if (target != Entity{})
                        {
                            if (target.HasComponent<SpriteComponent>())
                            {
                                auto& sprite = target.GetComponent<SpriteComponent>();
                                sprite.Texture = Texture2D::Create(String(path));
                                sprite.Color = LinearColor::White;
                            }
                        }
                    }
                }
            });

        m_PanelManager.AddPanel(m_MainViewport, false);


        m_SceneHierarchy = CreateRef<SceneHierarchyPanel>("SceneHierarchy", m_EditorScene);
        m_PanelManager.AddPanel(m_SceneHierarchy, Keyboard::J);

        auto contentBrowser = CreateRef<ContentBrowserPanel>("ContentBrowser");
        m_PanelManager.AddPanel(contentBrowser, Keyboard::Space);

        auto profiling = CreateRef<ProfilingPanel>("ProfilingPanel");
        m_PanelManager.AddPanel(profiling, Keyboard::K);

        m_EditorSettings = CreateRef<EditorSettingsPanel>("EditorSettings");
        m_PanelManager.AddPanel(m_EditorSettings, Keyboard::I);
    }

    Entity EditorLayer::GetEntityByCurrentMousePosition()
    {
        const auto& vpDesc = m_MainViewport->GetDescription();

        auto [mx, my] = ImGui::GetMousePos();
        mx -= vpDesc.Bounds[0].x;
        my -= vpDesc.Bounds[0].y;
        Vector2 viewportSize = vpDesc.Bounds[1] - vpDesc.Bounds[0];
        my = viewportSize.y - my;

        int mouseX = (int)mx;
        int mouseY = (int)my;

        if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
        {
            int pixelData = Renderer::GetFramebuffer()->ReadPixel(1, mouseX, mouseY);
            if (pixelData != -1)
                return { (entt::entity)pixelData, m_EditorScene.get() };
        }

        return Entity{};
    }

    void EditorLayer::RenderOverlay()
    {
        if (m_SceneState == SceneState::Play)
        {
            Entity camera = m_ActiveScene->GetPrimaryCameraEntity();
            auto& runtimeCamera = camera.GetComponent<CameraComponent>().Camera;

            Matrix4 viewProjection = Math::AffineInverse(camera.GetComponent<TransformComponent>().AsMatrix()) * runtimeCamera.GetProjectionMatrix();
            Renderer2D::BeginScene(viewProjection);
        }
        else
        {
            Renderer2D::BeginScene(m_EditorCamera.GetViewProjectionMatrix());
        }

        if (m_EditorSettings->GetSettings().m_ShowPhysicsColliders)
        {
            {
                auto view = m_EditorScene->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
                for (auto entity : view)
                {
                    const auto& [tc, bc2d] = view.get<TransformComponent, BoxCollider2DComponent>(entity);

                    Vector2 translation = Vector2(tc.Translation);
                    Vector2 scale = tc.Scale * Vector3(bc2d.Size * 2.f, 1.f);
                    Matrix4 transform = Math::ScaleMatrix(Vector3(scale)).Translate(Vector3(bc2d.Offset.y, bc2d.Offset.x, 0.f)).Rotate(tc.Rotation.z, Vector3::Back()).Translate(Vector3(translation, 0.001f));

                    Renderer2D::DrawRect(transform, LinearColor::Green, 3.f);
                }
            }

            {
                auto view = m_EditorScene->GetAllEntitiesWith<TransformComponent, CircleCollider2DComponent>();
                for (auto entity : view)
                {
                    const auto& [tc, cc2d] = view.get<TransformComponent, CircleCollider2DComponent>(entity);

                    Vector2 translation = Vector2(tc.Translation) + cc2d.Offset;
                    Vector2 scale = Vector2(tc.Scale) * Vector2(cc2d.Radius * 2);
                    Matrix4 transform = Math::ScaleMatrix(Vector3(scale)).Translate(Vector3(translation, 0.001f));

                    Renderer2D::DrawCircle(transform, LinearColor::Green, 0.05f * 0.5f / cc2d.Radius);
                }
            }
        }
        

        if (m_SelectedEntity)
        {
            const TransformComponent& transform = m_SelectedEntity.GetComponent<TransformComponent>();
            Renderer2D::DrawRect(transform.AsMatrix(), { 1.f, 0.5f, 0.f, 1.f }, 6.f);
        }

        Renderer2D::EndScene();
    }

    void EditorLayer::OnScenePlay()
    {
        const auto& vpDesc = m_MainViewport->GetDescription();

        if (m_SceneState == SceneState::Simulation)
            OnSceneStop();

        SelectEntity({});

        SaveSceneAs(m_TemporaryEditorScenePath);
        m_SceneState = SceneState::Play;

        m_RuntimeScene = m_EditorScene;
        m_RuntimeScene->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
        m_RuntimeScene->OnRuntimeStart();
        m_SceneHierarchy->SetContext(m_RuntimeScene);

        m_ActiveScene = m_RuntimeScene;
    }

    void EditorLayer::OnSceneStop()
    {
        m_SceneState = SceneState::Edit;

        m_ActiveScene = m_EditorScene;

        Filepath activeScenePath = m_CurrentScenePath;
        OpenScene(m_TemporaryEditorScenePath);
        m_CurrentScenePath = activeScenePath;

        m_SceneHierarchy->SetContext(m_EditorScene);

        m_RuntimeScene = nullptr;
    }

    void EditorLayer::OnSceneSimulate()
    {
        const auto& vpDesc = m_MainViewport->GetDescription();

        SelectEntity({});

        SaveSceneAs(m_TemporaryEditorScenePath);
        m_SceneState = SceneState::Simulation;

        m_RuntimeScene = m_EditorScene;
        m_RuntimeScene->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
        m_RuntimeScene->OnSimulationStart();
        m_SceneHierarchy->SetContext(m_RuntimeScene);

        m_ActiveScene = m_RuntimeScene;
    }

    void EditorLayer::OnEvent(Event& event)
    {
        const auto& vpDesc = m_MainViewport->GetDescription();
        if ((m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulation) && vpDesc.IsHovered && !ImGuizmo::IsUsing())
        {
            m_EditorCamera.OnEvent(event);
            m_ImGuizmoLayer.OnEvent(event);
        }

        m_PanelManager.OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(ATN_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonReleasedEvent>(ATN_BIND_EVENT_FN(EditorLayer::OnMouseReleased));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
    {
        if (event.IsRepeat())
            return false;

        bool ctrl = Input::IsKeyPressed(Keyboard::LCtrl) || Input::IsKeyPressed(Keyboard::RCtrl);

        switch (event.GetKeyCode())
        {
            // Scenes Management
        case Keyboard::S:
        {
            if (ctrl)
            {
                if (m_CurrentScenePath == Filepath())
                    SaveSceneAs();
                else
                    SaveSceneAs(m_CurrentScenePath);
            }
            break;
        }
        case Keyboard::N: if (ctrl) NewScene(); break;
        case Keyboard::O: if (ctrl) OpenScene(); break;
            
        case Keyboard::F4: if (m_SceneState == SceneState::Play || m_SceneState == SceneState::Simulation) OnSceneStop(); break;
        case Keyboard::F5: if (m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulation) OnScenePlay(); break;
        case Keyboard::F6: if (m_SceneState == SceneState::Edit) OnSceneSimulate(); break;

            //Entities
        case Keyboard::Escape: if (m_SelectedEntity) SelectEntity({});; break;
        case Keyboard::Delete:
        {
            if (m_SelectedEntity && m_SceneState == SceneState::Edit)
            {
                m_EditorScene->DestroyEntity(m_SelectedEntity);
                SelectEntity({});
            }
            break;
        }
        case Keyboard::F11:
        {
            auto& window = Application::Get().GetWindow();
            WindowMode currentMode = window.GetWindowMode();
            if (currentMode == WindowMode::Fullscreen)
            {
                window.SetWindowMode(WindowMode::Maximized);
                m_MainMenuBar->UseWindowDefaultButtons(false);
            }
            else
            {
                window.SetWindowMode(WindowMode::Fullscreen);
                m_MainMenuBar->UseWindowDefaultButtons(true);
            }
            break;
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
            if (m_SceneState != SceneState::Play)
            {
                Entity selectedEntity = GetEntityByCurrentMousePosition();
                if (selectedEntity)
                {
                    SelectEntity(selectedEntity);
                }
            }
            break;
        }
        }
        return false;
    }

    void EditorLayer::NewScene()
    {
        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        m_CurrentScenePath = Filepath();
        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;
        m_SceneHierarchy->SetContext(m_ActiveScene);
        ATN_CORE_TRACE("Successfully created new scene");
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

    void EditorLayer::SaveSceneAs(const Filepath& path)
    {
        SceneSerializer serializer(m_ActiveScene);
        serializer.SerializeToFile(path.string());
        ATN_CORE_TRACE("Successfully saved Scene into '{0}'", path.string().data());
    }

    void EditorLayer::OpenScene()
    {
        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        String filepath = FileDialogs::OpenFile("Athena Scene (*atn)\0*.atn\0");
        if (!filepath.empty())
            OpenScene(filepath);
        else
            ATN_CORE_ERROR("Invalid filepath to loaded Scene '{0}'", filepath.data());
    }

    void EditorLayer::OpenScene(const Filepath& path)
    {
        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        Ref<Scene> newScene = CreateRef<Scene>();

        SceneSerializer serializer(newScene);
        if(serializer.DeserializeFromFile(path.string()))
        {
            m_CurrentScenePath = path;

            m_EditorScene = newScene;
            m_ActiveScene = newScene;

            m_MainMenuBar->SetSceneRef(m_EditorScene);
            m_SceneHierarchy->SetContext(m_ActiveScene);
            SelectEntity({});
            ATN_CORE_TRACE("Successfully load Scene from '{0}'", path.string().data());
        }
        else
        {
            ATN_CORE_ERROR("Failed to load Scene from '{0}'", path.string().data());
        }
    }
}
