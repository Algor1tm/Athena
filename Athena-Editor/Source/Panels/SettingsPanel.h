#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Renderer/SceneRenderer.h"
#include "Panels/Panel.h"


namespace Athena
{
	struct EditorSettings
	{
		bool ShowPhysicsColliders = false;
		float CameraSpeedLevel = 1.f;
		bool ReloadScriptsOnStart = true;
	};


	class SettingsPanel: public Panel
	{
	public:
		SettingsPanel(std::string_view name, const Ref<EditorContext>& context);

		virtual void OnImGuiRender() override;
		void SetContext(const Ref<SceneRenderer>& renderer) { m_ViewportRenderer = renderer; }
		const EditorSettings& GetEditorSettings() { return m_EditorSettings; }

	private:
		Ref<SceneRenderer> m_ViewportRenderer;
		EditorSettings m_EditorSettings;
	};
}
