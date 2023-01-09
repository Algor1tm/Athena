#include "ImGuizmoLayer.h"

#include "Athena/Input/Input.h"

#include "Panels/ViewportPanel.h"


namespace Athena
{
	ImGuizmoLayer::ImGuizmoLayer(EditorCamera* camera)
		: m_pCamera(camera)
	{

	}

	void ImGuizmoLayer::OnImGuiRender() 
	{
        if (m_pCamera && m_pViewportPanel && m_ActiveEntity && m_GuizmoOperation != ImGuizmo::OPERATION::BOUNDS && m_ActiveEntity.HasComponent<TransformComponent>())
        {
            auto& desc = m_pViewportPanel->GetDescription();
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(desc.Bounds[0].x, desc.Bounds[0].y,
                desc.Bounds[1].x - desc.Bounds[0].x, desc.Bounds[1].y - desc.Bounds[0].y);

            //auto cameraEntity = m_ActiveScene->GetPrimaryCameraEntity();
            //const auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
            //const Matrix4& cameraProjection = camera.GetProjection();
            //Matrix4 cameraView = Math::AffineInverse(cameraEntity.GetComponent<TransformComponent>().AsMatrix());
            const Matrix4& cameraProjection = m_pCamera->GetProjectionMatrix();
            const Matrix4& cameraView = m_pCamera->GetViewMatrix();

            auto& tc = m_ActiveEntity.GetComponent<TransformComponent>();
            Matrix4 transform = tc.AsMatrix();

            bool snap = Input::IsKeyPressed(Keyboard::LCtrl);
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
	}

    void ImGuizmoLayer::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>(ATN_BIND_EVENT_FN(ImGuizmoLayer::OnKeyPressedEvent));
    }

    bool ImGuizmoLayer::OnKeyPressedEvent(KeyPressedEvent& event)
    {
        if (m_pViewportPanel)
        {
            auto& desc = m_pViewportPanel->GetDescription();
            switch (event.GetKeyCode())
            {
            case Keyboard::Q: if (m_ActiveEntity && (desc.IsHovered || desc.IsFocused))(m_GuizmoOperation = ImGuizmo::OPERATION::BOUNDS); break;
            case Keyboard::W: if (m_ActiveEntity && (desc.IsHovered || desc.IsFocused))(m_GuizmoOperation = ImGuizmo::OPERATION::TRANSLATE); break;
            case Keyboard::E: if (m_ActiveEntity && (desc.IsHovered || desc.IsFocused))(m_GuizmoOperation = ImGuizmo::OPERATION::ROTATE); break;
            case Keyboard::R: if (m_ActiveEntity && (desc.IsHovered || desc.IsFocused))(m_GuizmoOperation = ImGuizmo::OPERATION::SCALE); break;
            }
        }

        return false;
    }
}
