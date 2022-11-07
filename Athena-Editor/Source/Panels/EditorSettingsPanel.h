#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Panel.h"


namespace Athena
{
	struct EditorSettings
	{
		bool m_ShowPhysicsColliders = false;
	};


	class EditorSettingsPanel: public Panel
	{
	public:
		EditorSettingsPanel(std::string_view name);

		virtual void OnImGuiRender() override;
		const EditorSettings& GetSettings() { return m_Settings; }

	private:
		EditorSettings m_Settings;
	};
}
