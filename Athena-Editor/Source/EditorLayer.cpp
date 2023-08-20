#include "EditorLayer.h"

#include "Athena/Core/Application.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/PlatformUtils.h"

#include "Athena/ImGui/ImGuiLayer.h"

#include "Athena/Input/Input.h"

#include "Athena/Renderer/SceneRenderer.h"
#include "Athena/Renderer/SceneRenderer2D.h"
#include "Athena/Renderer/Renderer.h"

#include "Athena/Scene/Components.h"
#include "Athena/Scene/SceneSerializer.h"

#include "Panels/ContentBrowserPanel.h"
#include "Panels/SettingsPanel.h"
#include "Panels/MenuBarPanel.h"
#include "Panels/ProfilingPanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ViewportPanel.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>


namespace Athena
{
    EditorLayer* EditorLayer::s_Instance = nullptr;


    EditorLayer::EditorLayer(const EditorConfig& config)
        : Layer("EditorLayer"), m_Config(config)
    {
        ATN_CORE_ASSERT(s_Instance == nullptr, "EditorLayer already exists!");
        s_Instance = this;

        const FilePath& resources = m_Config.EditorResources;

        m_PlayIcon = Texture2D::Create(resources / "Icons/Editor/MenuBar/PlayIcon.png");
        m_SimulationIcon = Texture2D::Create(resources / "Icons/Editor/MenuBar/SimulationIcon.png");
        m_StopIcon = Texture2D::Create(resources / "Icons/Editor/MenuBar/StopIcon.png");

        m_Titlebar = CreateRef<Titlebar>(Application::Get().GetName());

        m_EditorCamera = CreateRef<FirstPersonCamera>(Math::Radians(50.f), 16.f / 9.f, 0.1f, 1000.f);
        m_ImGuizmoLayer.SetCamera(m_EditorCamera.get());
    }

    void EditorLayer::OnAttach()
    {
        Application::Get().GetImGuiLayer()->BlockEvents(false);

        Application::Get().GetWindow().SetTitlebarHitTestCallback([this]() { return m_Titlebar->IsHovered(); });

        InitializePanels();

        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;

#if 1
        OpenScene("Assets/Scenes/Default.atn");
#endif

        m_SceneHierarchy->SetContext(m_EditorScene);
    }

    void EditorLayer::OnDetach()
    {

    }

    void EditorLayer::OnUpdate(Time frameTime)
    {
        const auto& vpDesc = m_MainViewport->GetDescription();
        const auto& framebuffer = SceneRenderer::GetFinalFramebuffer();
        const auto& framebufferInfo = framebuffer->GetCreateInfo();

        if (vpDesc.Size.x > 0 && vpDesc.Size.y > 0 &&
            (framebufferInfo.Width != vpDesc.Size.x || framebufferInfo.Height != vpDesc.Size.y))
        {
            SceneRenderer::OnWindowResized(vpDesc.Size.x, vpDesc.Size.y);
            m_EditorCamera->SetViewportSize(vpDesc.Size.x, vpDesc.Size.y);
                
            m_ActiveScene->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
        }

        Renderer::ResetStats();

        SceneRenderer::BeginFrame();

        switch (m_SceneState)
        {
        case SceneState::Edit:
        {
            if ((m_HideCursor || vpDesc.IsHovered) && !ImGuizmo::IsUsing())
                m_EditorCamera->OnUpdate(frameTime);

            m_ActiveScene->OnUpdateEditor(frameTime, *m_EditorCamera); 
            break;
        }
        case SceneState::Play:
        {
            m_ActiveScene->OnUpdateRuntime(frameTime); 
            break;
        }
        case SceneState::Simulation:
        {
            if ((m_HideCursor || vpDesc.IsHovered) && !ImGuizmo::IsUsing())
                m_EditorCamera->OnUpdate(frameTime);

            m_ActiveScene->OnUpdateSimulation(frameTime, *m_EditorCamera);
            break;
        }
        }

        RenderOverlay();
        SceneRenderer::EndFrame();

        SelectEntity(m_SceneHierarchy->GetSelectedEntity());
    }

    void EditorLayer::OnImGuiRender()
    {
        const bool isMaximized = Application::Get().GetWindow().GetWindowMode() == WindowMode::Maximized;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, isMaximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });

        ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);

        ImGui::PopStyleVar(2);

        m_Titlebar->OnImGuiRender();

        ImGui::SetCursorPosY(m_Titlebar->GetHeight());
        // Dockspace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 300.0f;
        ImGui::DockSpace(ImGui::GetID("MyDockspace"));
        style.WindowMinSize.x = minWinSizeX;

        m_PanelManager.OnImGuiRender();

        ImGui::End();

        m_EditorCamera->SetMoveSpeedLevel(m_SettingsPanel->GetEditorSettings().CameraSpeedLevel);
        m_MainViewport->SetFramebuffer(SceneRenderer::GetFinalFramebuffer(), 0);
    }

    void EditorLayer::SelectEntity(Entity entity)
    {
        m_ImGuizmoLayer.SetActiveEntity(entity);
        m_SceneHierarchy->SetSelectedEntity(entity);
        m_SelectedEntity = entity;
    }

    Entity EditorLayer::DuplicateEntity(Entity entity)
    {
        if (entity)
        {
			Entity newEntity = m_EditorScene->DuplicateEntity(entity);
			SelectEntity(newEntity);
			return newEntity;
        }

        return entity;
    }

    void EditorLayer::InitializePanels()
    {
        //m_MainMenuBar = CreateRef<MenuBarPanel>("MainMenuBar");
        //m_MainMenuBar->SetLogoIcon(Texture2D::Create(m_Config.EditorResources / "Icons/Editor/MenuBar/Logo-no-background.png"));
        //m_MainMenuBar->AddMenuItem("File", [this]()
        //    {
        //        if (ImGui::MenuItem("New", "Ctrl+N", false))
        //            NewScene();

        //        if (ImGui::MenuItem("Open...", "Ctrl+O", false))
        //            OpenScene();

        //        if (ImGui::MenuItem("Save As...", "Ctrl+S", false))
        //            SaveSceneAs();

        //        ImGui::Separator();
        //        ImGui::Spacing();

        //        if (ImGui::MenuItem("Exit", NULL, false))
        //            Application::Get().Close();
        //    });

        //m_MainMenuBar->AddMenuItem("View", [this]()
        //    {
        //        m_PanelManager.ImGuiRenderAsMenuItems();
        //    });

        //m_MainMenuBar->AddMenuButton(m_PlayIcon, [this](Ref<Texture2D>& currentIcon)
        //    {
        //        currentIcon = m_SceneState == SceneState::Edit ? m_StopIcon : m_PlayIcon;

        //        if (m_SceneState == SceneState::Edit)
        //            OnScenePlay();
        //        else if (m_SceneState == SceneState::Play)
        //            OnSceneStop();
        //    });

        //m_MainMenuBar->AddMenuButton(m_SimulationIcon, [this](Ref<Texture2D>& currentIcon)
        //    {
        //        currentIcon = m_SceneState == SceneState::Edit ? m_StopIcon : m_SimulationIcon;

        //        if (m_SceneState == SceneState::Edit)
        //            OnSceneSimulate();
        //        else if (m_SceneState == SceneState::Simulation)
        //            OnSceneStop();
        //    });

        //m_PanelManager.AddPanel(m_MainMenuBar, false);


        m_MainViewport = CreateRef<ViewportPanel>("MainViewport");

        m_MainViewport->SetImGuizmoLayer(&m_ImGuizmoLayer);
        m_MainViewport->SetFramebuffer(SceneRenderer::GetFinalFramebuffer(), 0);
        m_MainViewport->SetDragDropCallback([this]()
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    std::string_view path = (const char*)payload->Data;
                    FilePath ext = FilePath(path).extension();
                    //Scene Drag/Drop
                    if (m_SceneState == SceneState::Edit && ext == ".atn\0")
                    {
                        OpenScene(path.data());
                    }
                    //Texture Drag/Drop on Entity
                    else if (m_SceneState == SceneState::Edit && ext == ".png\0")
                    {
                        Entity target = GetEntityByCurrentMousePosition();
                        if (target != Entity{})
                        {
                            if (target.HasComponent<SpriteComponent>())
                            {
                                auto& sprite = target.GetComponent<SpriteComponent>();
                                sprite.Texture = Texture2D::Create(FilePath(path));
                                sprite.Color = LinearColor::White;
                            }
                        }
                    }
                    // Mesh Drag/Drop
                    else if (m_SceneState == SceneState::Edit && (ext == ".obj\0" || ext == ".fbx" || ext == ".x3d" || ext == ".gltf" || ext == ".blend"))
                    {
                        Ref<StaticMesh> mesh = StaticMesh::Create(path);
                        if (mesh)
                        {
                            Entity entity = m_ActiveScene->CreateEntity();
                            entity.GetComponent<TagComponent>().Tag = mesh->GetName();
                            entity.AddComponent<StaticMeshComponent>().Mesh = mesh;
                            SelectEntity(entity);
                        }
                    }
                }
            });

        m_PanelManager.AddPanel(m_MainViewport, false);

        m_SettingsPanel = CreateRef<SettingsPanel>("Settings");
		m_PanelManager.AddPanel(m_SettingsPanel, Keyboard::I);

        m_SceneHierarchy = CreateRef<SceneHierarchyPanel>("SceneHierarchy", m_EditorScene);
        m_PanelManager.AddPanel(m_SceneHierarchy, Keyboard::J);

        auto contentBrowser = CreateRef<ContentBrowserPanel>("ContentBrowser");
        m_PanelManager.AddPanel(contentBrowser, Keyboard::Space);

        auto profiling = CreateRef<ProfilingPanel>("ProfilingPanel");
        m_PanelManager.AddPanel(profiling, Keyboard::K);
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
            int pixelData = SceneRenderer::GetEntityIDFramebuffer()->ReadPixel(0, mouseX, mouseY);
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
            if (camera)
            {
				auto& runtimeCamera = camera.GetComponent<CameraComponent>().Camera;

				Matrix4 view = Math::AffineInverse(camera.GetComponent<TransformComponent>().AsMatrix());
                SceneRenderer2D::BeginScene(view, runtimeCamera.GetProjectionMatrix());
            }
            else
            {
                SceneRenderer2D::BeginScene(Matrix4::Identity(), m_EditorCamera->GetProjectionMatrix());
            }
        }
        else
        {
            SceneRenderer2D::BeginScene(m_EditorCamera->GetViewMatrix(), m_EditorCamera->GetProjectionMatrix());
        }

        if (m_SettingsPanel->GetEditorSettings().ShowPhysicsColliders)
        {
            {
                auto view = m_EditorScene->GetAllEntitiesWith<TransformComponent, BoxCollider2DComponent>();
                for (auto entity : view)
                {
                    const auto& [tc, bc2d] = view.get<TransformComponent, BoxCollider2DComponent>(entity);

                    Vector2 translation = Vector2(tc.Translation);
                    Vector2 scale = tc.Scale * Vector3(bc2d.Size * 2.f, 1.f);
                    Matrix4 transform = Math::ScaleMatrix(Vector3(scale)).Translate(Vector3(bc2d.Offset.y, bc2d.Offset.x, 0.f)).Rotate(tc.Rotation.z, Vector3::Back()).Translate(Vector3(translation, 0.001f));

                    SceneRenderer2D::DrawRect(transform, LinearColor::Green, 3.f);
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

                    SceneRenderer2D::DrawCircle(transform, LinearColor::Green, 0.05f * 0.5f / cc2d.Radius);
                }
            }
        }
        

        if (m_SelectedEntity)
        {
            LinearColor color = { 1.f, 0.5f, 0.f, 1.f };
            TransformComponent worldTransform = m_SelectedEntity.GetWorldTransform();

            if (m_SelectedEntity.HasComponent<CameraComponent>())
            {
                float distance = Math::Distance(m_EditorCamera->GetPosition(), worldTransform.Translation);
                float scale = 0.1f * distance;
                Matrix4 rectTransform = Math::ConstructTransform(worldTransform.Translation, {scale, scale, scale}, worldTransform.Rotation);
                SceneRenderer2D::DrawRect(rectTransform, color, 1);
            }
            else if (m_SelectedEntity.HasComponent<StaticMeshComponent>())
            {
                const AABB& box = m_SelectedEntity.GetComponent<StaticMeshComponent>().Mesh->GetBoundingBox();
                
                AABB transformedBox = box.Transform(worldTransform.AsMatrix());

                Vector3 min = transformedBox.GetMinPoint();
                Vector3 max = transformedBox.GetMaxPoint();

                // front
                SceneRenderer2D::DrawLine(max, { max.x, min.y, max.z }, color);
                SceneRenderer2D::DrawLine({ max.x, min.y, max.z }, { min.x, min.y, max.z }, color);
                SceneRenderer2D::DrawLine({ min.x, min.y, max.z }, { min.x, max.y, max.z }, color);
                SceneRenderer2D::DrawLine({ min.x, max.y, max.z }, max, color);
                // back
                SceneRenderer2D::DrawLine(min, { min.x, max.y, min.z }, color);
                SceneRenderer2D::DrawLine({ min.x, max.y, min.z }, { max.x, max.y, min.z }, color);
                SceneRenderer2D::DrawLine({ max.x, max.y, min.z }, { max.x, min.y, min.z }, color);
                SceneRenderer2D::DrawLine({ max.x, min.y, min.z }, min, color);

                // left
                SceneRenderer2D::DrawLine(min, { min.x, min.y, max.z }, color);
                SceneRenderer2D::DrawLine({ min.x, max.y, min.z }, { min.x, max.y, max.z }, color);

                // right
                SceneRenderer2D::DrawLine(max, { max.x, max.y, min.z }, color);
                SceneRenderer2D::DrawLine({ max.x, min.y, max.z }, { max.x, min.y, min.z }, color);
            }
            else if(m_SelectedEntity.HasComponent<SpriteComponent>() || m_SelectedEntity.HasComponent<CircleComponent>())
            {
                SceneRenderer2D::DrawRect(worldTransform.AsMatrix(), color, 8.f);
            }
            else if (m_SelectedEntity.HasComponent<PointLightComponent>())
            {
                const Vector3& position = worldTransform.Translation;
                float radius = m_SelectedEntity.GetComponent<PointLightComponent>().Radius;

                constexpr float step = Math::Radians(10.f);
                for (float angle = 0.f; angle < 2 * Math::PI<float>() - step; angle += step)
                {
                    Vector2 offset0 = { radius * Math::Cos(angle), radius * Math::Sin(angle) };
                    Vector2 offset1 = { radius * Math::Cos(angle + step), radius * Math::Sin(angle + step) };

                    Vector3 p0 = position + Vector3(offset0.x, offset0.y, 0.f );
                    Vector3 p1 = position + Vector3(offset1.x, offset1.y, 0.f);

                    SceneRenderer2D::DrawLine(p0, p1, color);

					p0 = position + Vector3(offset0.x, 0.f, offset0.y);
					p1 = position + Vector3(offset1.x, 0.f, offset1.y);

                    SceneRenderer2D::DrawLine(p0, p1, color);

					p0 = position + Vector3(0.f, offset0.x, offset0.y);
					p1 = position + Vector3(0.f, offset1.x, offset1.y);

                    SceneRenderer2D::DrawLine(p0, p1, color);
                }
            }
        }

        SceneRenderer2D::EndScene();
    }

    void EditorLayer::OnScenePlay()
    {
        if (m_SettingsPanel->GetEditorSettings().ReloadScriptsOnStart)
        {
            m_ActiveScene->LoadAllScripts();
        }

        const auto& vpDesc = m_MainViewport->GetDescription();

        if (m_SceneState == SceneState::Simulation)
            OnSceneStop();

        m_SceneState = SceneState::Play;

        m_RuntimeScene = Scene::Copy(m_EditorScene);
        m_RuntimeScene->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
        m_RuntimeScene->OnRuntimeStart();
        m_SceneHierarchy->SetContext(m_RuntimeScene);

        m_ActiveScene = m_RuntimeScene;
    }

    void EditorLayer::OnSceneStop()
    {
        m_SceneState = SceneState::Edit;

        m_ActiveScene = m_EditorScene;
        m_SceneHierarchy->SetContext(m_EditorScene);
        m_RuntimeScene = nullptr;
    }

    void EditorLayer::OnSceneSimulate()
    {
        const auto& vpDesc = m_MainViewport->GetDescription();

        m_SceneState = SceneState::Simulation;

        m_RuntimeScene = Scene::Copy(m_EditorScene);
        m_RuntimeScene->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
        m_RuntimeScene->OnSimulationStart();
        m_SceneHierarchy->SetContext(m_RuntimeScene);

        m_ActiveScene = m_RuntimeScene;
    }

    void EditorLayer::OnEvent(Event& event)
    {
        const auto& vpDesc = m_MainViewport->GetDescription();
        if ((m_SceneState == SceneState::Edit || m_SceneState == SceneState::Simulation) && !ImGuizmo::IsUsing())
        {
            bool rightMB = Input::IsMouseButtonPressed(Mouse::Right);

            if (vpDesc.IsHovered)
            {
				m_ImGuizmoLayer.OnEvent(event);
				m_EditorCamera->OnEvent(event);

                if(rightMB && !m_HideCursor)
				{
					m_HideCursor = true;
					Application::Get().GetWindow().HideCursor(m_HideCursor);
				}
            }

            if (!rightMB && m_HideCursor)
            {
				m_HideCursor = false;
				Application::Get().GetWindow().HideCursor(m_HideCursor);
            }
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
                if (m_CurrentScenePath == FilePath())
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
		case Keyboard::D: if (ctrl) DuplicateEntity(m_SelectedEntity); break;
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
            }
            else
            {
                window.SetWindowMode(WindowMode::Fullscreen);
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

        m_CurrentScenePath = FilePath();

        m_RuntimeScene = nullptr;
        m_EditorScene = CreateRef<Scene>();
        m_ActiveScene = m_EditorScene;

        m_SceneHierarchy->SetContext(m_ActiveScene);
        SelectEntity(Entity{});

        ATN_CORE_TRACE("Successfully created new scene");
    }

    void EditorLayer::SaveSceneAs()
    {
        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        FilePath filepath = FileDialogs::SaveFile("Athena Scene (*atn)\0*.atn\0");
        if (!filepath.empty())
            SaveSceneAs(filepath);
        else
            ATN_CORE_ERROR("Invalid filepath to save Scene '{0}'", filepath.string());
    }

    void EditorLayer::SaveSceneAs(const FilePath& path)
    {
        SceneSerializer serializer(m_ActiveScene);
        serializer.SerializeToFile(path.string());
        ATN_CORE_TRACE("Successfully saved Scene into '{0}'", path.string());
    }

    void EditorLayer::OpenScene()
    {
        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        FilePath filepath = FileDialogs::OpenFile("Athena Scene (*atn)\0*.atn\0");
        if (!filepath.empty())
            OpenScene(filepath);
        else
            ATN_CORE_ERROR("Invalid filepath to loaded Scene '{0}'", filepath.string());
    }

    void EditorLayer::OpenScene(const FilePath& path)
    {
        if (m_SceneState != SceneState::Edit)
            OnSceneStop();

        Ref<Scene> newScene = CreateRef<Scene>();

        SceneSerializer serializer(newScene);
        if(serializer.DeserializeFromFile(path.string()))
        {
            m_CurrentScenePath = path;

            m_RuntimeScene = nullptr;
            m_EditorScene = newScene;
            m_ActiveScene = newScene;

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
