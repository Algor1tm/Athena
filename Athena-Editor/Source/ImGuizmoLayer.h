#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Input/KeyEvent.h"

#include "Athena/Scene/Entity.h"

#include <ImGui/imgui.h>
#include <ImGuizmo/ImGuizmo.h>


namespace Athena
{
	class ViewportPanel;
	class EditorCamera;


	class ImGuizmoLayer
	{
	public:
		friend class ViewportPanel;

	public:
		void OnImGuiRender();
		void OnEvent(Event& event);

		void SetCamera(EditorCamera* camera) { m_Camera = camera; }
		void SetActiveEntity(Entity entity) { m_ActiveEntity = entity; }

	private:
		bool OnKeyPressedEvent(KeyPressedEvent& event);

	private:
		ImGuizmo::OPERATION m_GuizmoOperation = ImGuizmo::OPERATION::TRANSLATE;

		ViewportPanel* m_ViewportPanel = nullptr;
		EditorCamera* m_Camera = nullptr;
		Entity m_ActiveEntity;
	};
}
