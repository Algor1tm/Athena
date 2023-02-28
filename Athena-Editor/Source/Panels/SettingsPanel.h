#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Renderer/Renderer.h"

#include "Panels/Panel.h"


namespace Athena
{
	struct EditorSettings
	{
		bool m_ShowPhysicsColliders = false;
		float m_CameraSpeedLevel = 1.f;
	};


	class SettingsPanel: public Panel
	{
	public:
		SettingsPanel(std::string_view name);

		virtual void OnImGuiRender() override;
		const EditorSettings& GetEditorSettings() { return m_EditorSettings; }

	private:
		EditorSettings m_EditorSettings;
		int32 m_RenderQueueLimit = -1;
		uint32 m_RenderGeometryCount = 0;
	};
}
