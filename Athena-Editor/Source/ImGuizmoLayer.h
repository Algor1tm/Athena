#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Input/Events/KeyEvent.h"

#include "Athena/Renderer/EditorCamera.h"
#include "Athena/Scene/Entity.h"

#include <ImGui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>


namespace Athena
{
	class ATHENA_API ImGuizmoLayer
	{
	public:
		friend class ATHENA_API ViewportPanel;

	public:
		ImGuizmoLayer(EditorCamera* camera);

		void OnImGuiRender();
		void OnEvent(Event& event);

		void SetCamera(EditorCamera* camera) { m_pCamera = camera; }
		void SetActiveEntity(Entity entity) { m_ActiveEntity = entity; }

	private:
		bool OnKeyPressedEvent(KeyPressedEvent& event);

	private:
		ImGuizmo::OPERATION m_GuizmoOperation = ImGuizmo::OPERATION::TRANSLATE;

		ViewportPanel* m_pViewportPanel = nullptr;
		EditorCamera* m_pCamera = nullptr;
		Entity m_ActiveEntity;
	};
}
