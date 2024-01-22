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

            Matrix4 cameraProjection = m_Camera->GetProjectionMatrix();
            const Matrix4& cameraView = m_Camera->GetViewMatrix();

            TransformComponent worldTransform = m_EditorCtx->SelectedEntity.GetWorldTransform();
            Matrix4 worldTransformMatrix = worldTransform.AsMatrix();

            bool snap = Input::IsKeyPressed(Keyboard::LCtrl);
            float snapValue = 0.5f;
            if (m_GuizmoOperation == ImGuizmo::OPERATION::ROTATE)
                snapValue = 45.f;

            Vector3 snapValues = Vector3(snapValue);

            ImGuizmo::Manipulate(cameraView.Data(), cameraProjection.Data(),
                m_GuizmoOperation, ImGuizmo::LOCAL, worldTransformMatrix.Data(),
                nullptr, snap ? snapValues.Data() : nullptr);

            if (ImGuizmo::IsUsing())
            {
                Vector3 translation, rotation, scale;
                Math::DecomposeTransform(worldTransformMatrix, translation, rotation, scale);

                TransformComponent newWorldTransform;
                newWorldTransform.Translation = translation;
                newWorldTransform.Rotation = rotation;
                newWorldTransform.Scale = scale;

               m_EditorCtx->SelectedEntity.GetComponent<TransformComponent>().ConvertToLocalTransform(newWorldTransform, worldTransform);
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
