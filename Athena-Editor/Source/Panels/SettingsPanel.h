#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Panels/Panel.h"


namespace Athena
{
	class SettingsPanel: public Panel
	{
	public:
		SettingsPanel(const Ref<EditorContext>& context);

		virtual void OnImGuiRender() override;

	private:
	};
}
