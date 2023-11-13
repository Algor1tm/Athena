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

#include "Athena/UI/Theme.h"

#include "Panels/PanelManager.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/SettingsPanel.h"
#include "Panels/ProfilingPanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ViewportPanel.h"

#include "EditorResources.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>


namespace Athena
{
    EditorLayer::EditorLayer(const EditorConfig& config)
        : Layer("EditorLayer"), m_Config(config)
    {

    }

    void EditorLayer::OnAttach()
    {
        m_EditorCtx = Ref<EditorContext>::Create();
        m_EditorCtx->ActiveScene = nullptr;
        m_EditorCtx->SceneState = SceneState::Edit;
        m_EditorCtx->SelectedEntity = {};

        m_EditorScene = Ref<Scene>::Create();
        m_EditorCtx->ActiveScene = m_EditorScene;

        m_ViewportRenderer = SceneRenderer::Create();

        //m_EditorCamera = Ref<FirstPersonCamera>::Create(Math::Radians(50.f), 16.f / 9.f, 0.1f, 1000.f);
        m_EditorCamera = Ref<OrthographicCamera>::Create(-1.f, 1.f, -1.f, 1.f, true);

        EditorResources::Init(m_Config.EditorResources);

        Application::Get().GetImGuiLayer()->BlockEvents(false);
        Application::Get().GetWindow().SetTitlebarHitTestCallback([this]() { return m_Titlebar->IsHovered(); });

        m_ImGuizmoLayer = Ref<ImGuizmoLayer>::Create(m_EditorCtx, m_EditorCamera);
        InitUI();
#if 0
        OpenScene("Assets/Scenes/Default.atn");
#endif
    }

    void EditorLayer::OnDetach()
    {
        m_ViewportRenderer->Shutdown();
        PanelManager::Shutdown();
        EditorResources::Shutdown();
        Application::Get().GetWindow().SetTitlebarHitTestCallback(nullptr);
    }

    void EditorLayer::OnUpdate(Time frameTime)
    {
        ATN_PROFILE_FUNC()

        auto viewportPanel = PanelManager::GetPanel<ViewportPanel>(VIEWPORT_PANEL_ID);

        const auto& vpDesc = viewportPanel->GetDescription();
        const auto& rendererSize = m_ViewportRenderer->GetViewportSize();

        if (vpDesc.Size.x > 0 && vpDesc.Size.y > 0 &&
            (rendererSize.x != vpDesc.Size.x || rendererSize.y != vpDesc.Size.y))
        {
            ATN_PROFILE_SCOPE("EditorLayer::OnViewportResize")

            m_ViewportRenderer->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
            m_EditorCamera->SetViewportSize(vpDesc.Size.x, vpDesc.Size.y);
            m_EditorCtx->ActiveScene->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
        }

        if ((m_HideCursor || vpDesc.IsHovered) && !ImGuizmo::IsUsing())
            m_EditorCamera->OnUpdate(frameTime);

        CameraInfo cameraInfo;
        cameraInfo.ViewMatrix = m_EditorCamera->GetViewMatrix();
        cameraInfo.ProjectionMatrix = m_EditorCamera->GetProjectionMatrix();
        cameraInfo.NearClip = m_EditorCamera->GetNearClip();
        cameraInfo.FarClip = m_EditorCamera->GetFarClip();

        m_ViewportRenderer->Render(cameraInfo);

        //Renderer::ResetStats();

        //switch (m_EditorCtx->SceneState)
        //{
        //case SceneState::Edit:
        //{
        //    if ((m_HideCursor || vpDesc.IsHovered) && !ImGuizmo::IsUsing())
        //        m_EditorCamera->OnUpdate(frameTime);

        //    m_EditorCtx->ActiveScene->OnUpdateEditor(frameTime, *m_EditorCamera);
        //    break;
        //}
        //case SceneState::Play:
        //{
        //    m_EditorCtx->ActiveScene->OnUpdateRuntime(frameTime);
        //    break;
        //}
        //case SceneState::Simulation:
        //{
        //    if ((m_HideCursor || vpDesc.IsHovered) && !ImGuizmo::IsUsing())
        //        m_EditorCamera->OnUpdate(frameTime);

        //    m_EditorCtx->ActiveScene->OnUpdateSimulation(frameTime, *m_EditorCamera);
        //    break;
        //}
        //}

        //RenderOverlay();
    }

    void EditorLayer::OnImGuiRender()
    {
        ATN_PROFILE_FUNC()

        const bool isMaximized = Application::Get().GetWindow().GetWindowMode() == WindowMode::Maximized;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, isMaximized ? ImVec2(6.0f, 6.0f) : ImVec2(1.0f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, UI::GetTheme().Titlebar);
        //ImGui::PushStyleColor(ImGuiCol_Border, UI::GetTheme().Accent);

        ImGui::Begin("DockSpaceWindow", nullptr, window_flags);

        //ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        m_Titlebar->OnImGuiRender();
        ImGui::SetCursorPosY(m_Titlebar->GetHeight());

        // Dockspace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 300.0f;
        ImGui::DockSpace(ImGui::GetID("MyDockspace"));
        style.WindowMinSize.x = minWinSizeX;

        PanelManager::OnImGuiRender();

        if (m_AboutModalOpen)
            DrawAboutModal();

        if (m_ThemeEditorOpen)
            DrawThemeEditor();

        ImGui::End();

        auto settingsPanel = PanelManager::GetPanel<SettingsPanel>(SETTINGS_PANEL_ID);
        m_EditorCamera->SetMoveSpeedLevel(settingsPanel->GetEditorSettings().CameraSpeedLevel);
    }

    Entity EditorLayer::DuplicateEntity(Entity entity)
    {
        if (entity && m_EditorCtx->SceneState == SceneState::Edit)
        {
			Entity newEntity = m_EditorScene->DuplicateEntity(entity);
            m_EditorCtx->SelectedEntity = newEntity;
			return newEntity;
        }

        return entity;
    }

    void EditorLayer::InitUI()
    {
        m_Titlebar = Ref<Titlebar>::Create(Application::Get().GetConfig().Name, m_EditorCtx);

        m_Titlebar->SetMenubarCallback([this]()
            {
                if (ImGui::BeginMenu("File"))
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

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View"))
                {
                    PanelManager::ImGuiRenderAsMenuItems();
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Edit"))
                {
                    if (ImGui::MenuItem("Theme Editor"))
                    {
                        m_ThemeEditorOpen = !m_ThemeEditorOpen;
                    }

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Help"))
                {
                    if (ImGui::MenuItem("About", NULL, false))
                    {
                        m_AboutModalOpen = true;
                    }
                    
                    ImGui::EndMenu();
                }
            });

        auto viewportPanel = Ref<ViewportPanel>::Create(VIEWPORT_PANEL_ID, m_EditorCtx);

        viewportPanel->SetImGuizmoLayer(m_ImGuizmoLayer);
        viewportPanel->SetViewportRenderer(m_ViewportRenderer);
        viewportPanel->SetDragDropCallback([this]()
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                {
                    std::string_view path = (const char*)payload->Data;
                    FilePath ext = FilePath(path).extension();
                    //Scene Drag/Drop
                    if (m_EditorCtx->SceneState == SceneState::Edit && ext == ".atn\0")
                    {
                        OpenScene(path.data());
                    }
                    //Texture Drag/Drop on Entity
                    else if (m_EditorCtx->SceneState == SceneState::Edit && ext == ".png\0")
                    {
                        Entity target = GetEntityByCurrentMousePosition();
                        if (target != Entity{})
                        {
                            if (target.HasComponent<SpriteComponent>())
                            {
                                auto& sprite = target.GetComponent<SpriteComponent>();
                                sprite.Texture = Texture2D::Create(FilePath(path));
                                sprite.Color = LinearColor::White;
                                m_EditorCtx->SelectedEntity = target;
                            }
                        }
                    }
                    // Mesh Drag/Drop
                    else if (m_EditorCtx->SceneState == SceneState::Edit && (ext == ".obj\0" || ext == ".fbx" || ext == ".x3d" || ext == ".gltf" || ext == ".blend"))
                    {
                        Ref<StaticMesh> mesh = StaticMesh::Create(path);
                        if (mesh)
                        {
                            Entity entity = m_EditorCtx->ActiveScene->CreateEntity();
                            entity.GetComponent<TagComponent>().Tag = mesh->GetName();
                            entity.AddComponent<StaticMeshComponent>().Mesh = mesh;
                            m_EditorCtx->SelectedEntity = entity;
                        }
                    }
                }
            });

        viewportPanel->SetUIOverlayCallback([this]()
            {
                float windowPaddingY = 3.f;

                ImColor rectColor = IM_COL32(0, 0, 0, 70);
                ImVec2 rectPadding = { -1.f, 0.f };

                float buttonPaddingX = -1.f;
                ImVec2 buttonSize = { 28.f, 26.f };
                int32 buttonCount = 2;

                ImDrawList* drawList = ImGui::GetWindowDrawList();
                float avail = ImGui::GetContentRegionAvail().x;

                float fullSize = 2 * rectPadding.x + buttonCount * buttonSize.x + (buttonCount - 1) * buttonPaddingX;
                ImVec2 rectMin = { (avail - fullSize) * 0.5f, windowPaddingY };
                rectMin.x = avail * 0.5f;
                rectMin.x += ImGui::GetCursorScreenPos().x;
                rectMin.y += ImGui::GetCursorScreenPos().y;

                drawList->AddRectFilled(rectMin, { rectMin.x + fullSize, rectMin.y + 2 * rectPadding.y + buttonSize.y }, rectColor, 4.f);

                ImColor tintNormal = IM_COL32(255, 255, 255, 255);

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { buttonPaddingX, 0.f });
                ImGui::SetCursorScreenPos({ rectMin.x + rectPadding.x, rectMin.y + rectPadding.y });

                Ref<Texture2D> playIcon = EditorResources::GetIcon("Viewport_Play");
                Ref<Texture2D> simulateIcon = EditorResources::GetIcon("Viewport_Simulate");
                Ref<Texture2D> stopIcon = EditorResources::GetIcon("Viewport_Stop");

                {
                    Ref<Texture2D> icon = m_EditorCtx->SceneState == SceneState::Edit || m_EditorCtx->SceneState == SceneState::Simulation ? playIcon : stopIcon;

                    if (ImGui::InvisibleButton("Play", buttonSize))
                    {
                        if (m_EditorCtx->SceneState == SceneState::Edit)
                            OnScenePlay();
                        else if (m_EditorCtx->SceneState == SceneState::Play)
                            OnSceneStop();
                    }

                    UI::ButtonImage(icon, tintNormal, tintNormal, tintNormal);
                }

                ImGui::SameLine();

                {
                    Ref<Texture2D> icon = m_EditorCtx->SceneState == SceneState::Edit || m_EditorCtx->SceneState == SceneState::Play ? simulateIcon : stopIcon;

                    if (ImGui::InvisibleButton("Simulate", buttonSize))
                    {
                        if (m_EditorCtx->SceneState == SceneState::Edit)
                            OnSceneSimulate();
                        else if (m_EditorCtx->SceneState == SceneState::Simulation)
                            OnSceneStop();
                    }

                    UI::ButtonImage(icon, tintNormal, tintNormal, tintNormal);
                }

                ImGui::PopStyleVar();
            });

        PanelManager::AddPanel(viewportPanel, false);

        auto settingsPanel = Ref<SettingsPanel>::Create(SETTINGS_PANEL_ID, m_EditorCtx);
        PanelManager::AddPanel(settingsPanel, Keyboard::I);

        auto sceneHierarchyPanel = Ref<SceneHierarchyPanel>::Create(SCENE_HIERARCHY_PANEL_ID, m_EditorCtx);
        PanelManager::AddPanel(sceneHierarchyPanel, Keyboard::J);

        auto contentBrowserPanel = Ref<ContentBrowserPanel>::Create(CONTENT_BORWSER_PANEL_ID, m_EditorCtx);
        PanelManager::AddPanel(contentBrowserPanel, Keyboard::Space);

        auto profilingPanel = Ref<ProfilingPanel>::Create(PROFILING_PANEL_ID, m_EditorCtx);
        profilingPanel->SetContext(m_ViewportRenderer);
        PanelManager::AddPanel(profilingPanel, Keyboard::K);
    }

    Entity EditorLayer::GetEntityByCurrentMousePosition()
    {
        //const auto& vpDesc = m_MainViewport->GetDescription();

        //auto [mx, my] = ImGui::GetMousePos();
        //mx -= vpDesc.Bounds[0].x;
        //my -= vpDesc.Bounds[0].y;
        //Vector2 viewportSize = vpDesc.Bounds[1] - vpDesc.Bounds[0];
        //my = viewportSize.y - my;

        //int mouseX = (int)mx;
        //int mouseY = (int)my;

        //if (mouseX >= 0 && mouseY >= 0 && mouseX < (int)viewportSize.x && mouseY < (int)viewportSize.y)
        //{
        //    int pixelData = SceneRenderer::GetEntityIDFramebuffer()->ReadPixel(0, mouseX, mouseY);
        //    if (pixelData != -1)
        //        return { (entt::entity)pixelData, m_EditorScene.get() };
        //}

        return Entity{};
    }

    void EditorLayer::RenderOverlay()
    {
        if (m_EditorCtx->SceneState == SceneState::Play)
        {
            Entity camera = m_EditorCtx->ActiveScene->GetPrimaryCameraEntity();
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

        auto settingsPanel = PanelManager::GetPanel<SettingsPanel>(SETTINGS_PANEL_ID);
        if (settingsPanel->GetEditorSettings().ShowPhysicsColliders)
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
        
        Entity selectedEntity = m_EditorCtx->SelectedEntity;
        if (selectedEntity)
        {
            LinearColor color = { 1.f, 0.5f, 0.f, 1.f };
            TransformComponent worldTransform = selectedEntity.GetWorldTransform();

            if (selectedEntity.HasComponent<CameraComponent>())
            {
                float distance = Math::Distance(m_EditorCamera->GetPosition(), worldTransform.Translation);
                float scale = 0.1f * distance;
                Matrix4 rectTransform = Math::ConstructTransform(worldTransform.Translation, {scale, scale, scale}, worldTransform.Rotation);
                SceneRenderer2D::DrawRect(rectTransform, color, 1);
            }
            else if (selectedEntity.HasComponent<StaticMeshComponent>())
            {
                const AABB& box = selectedEntity.GetComponent<StaticMeshComponent>().Mesh->GetBoundingBox();
                
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
            else if(selectedEntity.HasComponent<SpriteComponent>() || selectedEntity.HasComponent<CircleComponent>())
            {
                SceneRenderer2D::DrawRect(worldTransform.AsMatrix(), color, 8.f);
            }
            else if (selectedEntity.HasComponent<PointLightComponent>())
            {
                const Vector3& position = worldTransform.Translation;
                float radius = selectedEntity.GetComponent<PointLightComponent>().Radius;

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

    void EditorLayer::DrawAboutModal()
    {
        ImGui::OpenPopup("About");
        m_AboutModalOpen = ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (m_AboutModalOpen)
        {
            auto logo = EditorResources::GetIcon("Logo");
            UI::Image(logo, { 48, 48 });

            ImGui::SameLine();
            UI::ShiftCursorX(20.0f);

            ImGui::Text("Athena 3D Game Engine");

            if (UI::ButtonCentered("Close"))
            {
                m_AboutModalOpen = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void EditorLayer::DrawThemeEditor()
    {
        ImGui::Begin("Theme Editor");

        UI::Theme& theme = UI::GetTheme();

        UI::ThemeEditor(theme);

        if (ImGui::Button("Reset"))
            theme = UI::Theme::DefaultDark();

        Application::Get().GetImGuiLayer()->UpdateImGuiTheme();

        ImGui::SameLine();
        if (ImGui::Button("Close"))
            m_ThemeEditorOpen = false;

        ImGui::End();
    }

    void EditorLayer::OnScenePlay()
    {
        ATN_PROFILE_FUNC()

        auto settingsPanel = PanelManager::GetPanel<SettingsPanel>(SETTINGS_PANEL_ID);
        auto viewportPanel = PanelManager::GetPanel<ViewportPanel>(VIEWPORT_PANEL_ID);

        if (settingsPanel->GetEditorSettings().ReloadScriptsOnStart)
        {
            m_EditorCtx->ActiveScene->LoadAllScripts();
        }

        const auto& vpDesc = viewportPanel->GetDescription();

        if (m_EditorCtx->SceneState == SceneState::Simulation)
            OnSceneStop();

        m_EditorCtx->SceneState = SceneState::Play;

        m_RuntimeScene = Scene::Copy(m_EditorScene);
        m_RuntimeScene->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
        m_RuntimeScene->OnRuntimeStart();

        m_EditorCtx->ActiveScene = m_RuntimeScene;
        m_EditorCtx->SelectedEntity = {};
    }

    void EditorLayer::OnSceneStop()
    {
        ATN_PROFILE_FUNC()

        m_EditorCtx->SceneState = SceneState::Edit;

        m_EditorCtx->ActiveScene = m_EditorScene;
        m_RuntimeScene = nullptr;
    }

    void EditorLayer::OnSceneSimulate()
    {
        ATN_PROFILE_FUNC()

        auto viewportPanel = PanelManager::GetPanel<ViewportPanel>(VIEWPORT_PANEL_ID);
        const auto& vpDesc = viewportPanel->GetDescription();

        m_EditorCtx->SceneState = SceneState::Simulation;

        m_RuntimeScene = Scene::Copy(m_EditorScene);
        m_RuntimeScene->OnViewportResize(vpDesc.Size.x, vpDesc.Size.y);
        m_RuntimeScene->OnSimulationStart();

        m_EditorCtx->ActiveScene = m_RuntimeScene;
    }

    void EditorLayer::OnEvent(Event& event)
    {
        ATN_PROFILE_FUNC()

        auto viewportPanel = PanelManager::GetPanel<ViewportPanel>(VIEWPORT_PANEL_ID);
        const auto& vpDesc = viewportPanel->GetDescription();

        if ((m_EditorCtx->SceneState == SceneState::Edit || m_EditorCtx->SceneState == SceneState::Simulation) && !ImGuizmo::IsUsing())
        {
            bool rightMB = Input::IsMouseButtonPressed(Mouse::Right);

            if (vpDesc.IsHovered)
            {
				m_ImGuizmoLayer->OnEvent(event);
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

        PanelManager::OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(ATN_BIND_EVENT_FN(EditorLayer::OnKeyPressed));
        dispatcher.Dispatch<MouseButtonReleasedEvent>(ATN_BIND_EVENT_FN(EditorLayer::OnMouseReleased));
    }

    bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
    {
        if (event.IsRepeat())
            return false;

        bool ctrl = event.IsCtrlPressed();

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
            
        case Keyboard::F4: if (m_EditorCtx->SceneState == SceneState::Play || m_EditorCtx->SceneState == SceneState::Simulation) OnSceneStop(); break;
        case Keyboard::F5: if (m_EditorCtx->SceneState == SceneState::Edit || m_EditorCtx->SceneState == SceneState::Simulation) OnScenePlay(); break;
        case Keyboard::F6: if (m_EditorCtx->SceneState == SceneState::Edit) OnSceneSimulate(); break;
        
            //Entities
		case Keyboard::D: if (ctrl) DuplicateEntity(m_EditorCtx->SelectedEntity); break;
        case Keyboard::Escape: m_EditorCtx->SelectedEntity = {}; break;
        case Keyboard::Delete:
        {
            if (m_EditorCtx->SelectedEntity && m_EditorCtx->SceneState == SceneState::Edit)
            {
                m_EditorScene->DestroyEntity(m_EditorCtx->SelectedEntity);
                m_EditorCtx->SelectedEntity = {};
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
            if (m_EditorCtx->SceneState != SceneState::Play)
            {
                Entity selectedEntity = GetEntityByCurrentMousePosition();
                if (selectedEntity)
                {
                    m_EditorCtx->SelectedEntity = selectedEntity;
                }
            }
            break;
        }
        }
        return false;
    }

    void EditorLayer::NewScene()
    {
        if (m_EditorCtx->SceneState != SceneState::Edit)
            OnSceneStop();

        m_CurrentScenePath = FilePath();

        m_RuntimeScene = nullptr;
        m_EditorScene = Ref<Scene>::Create();
        m_EditorCtx->ActiveScene = m_EditorScene;
        m_EditorCtx->SelectedEntity = {};

        ATN_INFO_TAG("EditorLayer", "Successfully created new scene");
    }

    void EditorLayer::SaveSceneAs()
    {
        if (m_EditorCtx->SceneState != SceneState::Edit)
            OnSceneStop();

        FilePath filepath = FileDialogs::SaveFile("Athena Scene (*atn)\0*.atn\0");
        if (!filepath.empty())
            SaveSceneAs(filepath);
        else
            ATN_ERROR_TAG("EditorLayer", "Invalid filepath to save Scene '{}'", filepath.string());
    }

    void EditorLayer::SaveSceneAs(const FilePath& path)
    {
        SceneSerializer serializer(m_EditorCtx->ActiveScene);
        serializer.SerializeToFile(path.string());
        ATN_INFO_TAG("EditorLayer", "Successfully saved Scene into '{}'", path.string());
    }

    void EditorLayer::OpenScene()
    {
        if (m_EditorCtx->SceneState != SceneState::Edit)
            OnSceneStop();

        FilePath filepath = FileDialogs::OpenFile("Athena Scene (*atn)\0*.atn\0");
        if (!filepath.empty())
            OpenScene(filepath);
        else
            ATN_ERROR_TAG("EditorLayer", "Invalid filepath to loaded Scene '{}'", filepath.string());
    }

    void EditorLayer::OpenScene(const FilePath& path)
    {
        if (m_EditorCtx->SceneState != SceneState::Edit)
            OnSceneStop();

        Ref<Scene> newScene = Ref<Scene>::Create();

        SceneSerializer serializer(newScene);
        if(serializer.DeserializeFromFile(path.string()))
        {
            m_CurrentScenePath = path;
            ATN_INFO_TAG("EditorLayer", "Successfully load Scene from '{}'", path.string().data());
        }

        m_RuntimeScene = nullptr;
        m_EditorScene = newScene;
        m_EditorCtx->ActiveScene = newScene;
        m_EditorCtx->SelectedEntity = {};
    }
}
