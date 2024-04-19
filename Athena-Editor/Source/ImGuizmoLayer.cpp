#include "ImGuizmoLayer.h"

#include "Athena/Renderer/EditorCamera.h"
#include "Athena/Input/Input.h"
#include "Athena/Scene/Components.h"
#include "Panels/ViewportPanel.h"


namespace Athena
{
    ImGuizmoLayer::ImGuizmoLayer(const Ref<EditorContext>& context, const Ref<EditorCamera>& camera)
    {
        m_EditorCtx = context;
        m_Camera = camera;
    }

	void ImGuizmoLayer::OnImGuiRender() 
	{
        if (m_Camera && m_ViewportPanel && m_EditorCtx->SelectedEntity && m_GuizmoOperation != ImGuizmo::OPERATION::BOUNDS && m_EditorCtx->SelectedEntity.HasComponent<TransformComponent>())
        {
            auto& desc = m_ViewportPanel->GetDescription();
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(desc.Bounds[0].x, desc.Bounds[0].y,
                desc.Bounds[1].x - desc.Bounds[0].x, desc.Bounds[1].y - desc.Bounds[0].y);

            // Need to recreate projection matrix, because ImGuizmo 
            // does not work properly with reversed Z projection
            float fov = m_Camera->GetFOV();
            float znear = m_Camera->GetNearClip();
            float zfar = m_Camera->GetFarClip();
            float aspectRatio = m_Camera->GetAspectRatio();
            Matrix4 cameraProjection = Math::Perspective(fov, aspectRatio, znear, zfar);
            cameraProjection[1][1] = -cameraProjection[1][1]; // invert y

            const Matrix4& cameraView = m_Camera->GetViewMatrix();

            const WorldTransformComponent& worldTransform = m_EditorCtx->SelectedEntity.GetComponent<WorldTransformComponent>();
            Matrix4 worldTransformMatrix = worldTransform.AsMatrix();

            // Discard if not in viewport space
            Vector4 viewPos = Vector4(0, 0, 0, 1) * worldTransformMatrix * cameraView;
            if (viewPos.z > 0)
                return;

            bool snap = Input::IsKeyPressed(Keyboard::LCtrl);
            float snapValue = 0.5f;
            if (m_GuizmoOperation == ImGuizmo::OPERATION::ROTATE)
                snapValue = 45.f;

            Vector3 snapValues = Vector3(snapValue);
            ImGuizmo::MODE mode = m_EditorCtx->EditorSettings.GizmosLocalTransform ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD;

            ImGuizmo::Manipulate(cameraView.Data(), cameraProjection.Data(),
                m_GuizmoOperation, mode, worldTransformMatrix.Data(),
                nullptr, snap ? snapValues.Data() : nullptr);

            if (ImGuizmo::IsUsing())
            {
                Vector3 translation, rotation, scale;
                Math::DecomposeTransform(worldTransformMatrix, translation, rotation, scale);

                WorldTransformComponent newWorldTransform;
                newWorldTransform.Translation = translation;
                newWorldTransform.Rotation = rotation;
                newWorldTransform.Scale = scale;

               m_EditorCtx->SelectedEntity.GetComponent<TransformComponent>().UpdateLocalTransform(newWorldTransform, worldTransform);
            }
        }
	}

    void ImGuizmoLayer::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(ATN_BIND_EVENT_FN(ImGuizmoLayer::OnKeyPressedEvent));
    }

    bool ImGuizmoLayer::OnKeyPressedEvent(KeyPressedEvent& event)
    {
        if (m_ViewportPanel && !Input::IsMouseButtonPressed(Mouse::Right))
        {
            auto& desc = m_ViewportPanel->GetDescription();
            switch (event.GetKeyCode())
            {
            case Keyboard::Q: if (m_EditorCtx->SelectedEntity && (desc.IsHovered || desc.IsFocused))(m_GuizmoOperation = ImGuizmo::OPERATION::BOUNDS); break;
            case Keyboard::W: if (m_EditorCtx->SelectedEntity && (desc.IsHovered || desc.IsFocused))(m_GuizmoOperation = ImGuizmo::OPERATION::TRANSLATE); break;
            case Keyboard::E: if (m_EditorCtx->SelectedEntity && (desc.IsHovered || desc.IsFocused))(m_GuizmoOperation = ImGuizmo::OPERATION::ROTATE); break;
            case Keyboard::R: if (m_EditorCtx->SelectedEntity && (desc.IsHovered || desc.IsFocused))(m_GuizmoOperation = ImGuizmo::OPERATION::SCALE); break;
            }
        }

        return false;
    }
}
