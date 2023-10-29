#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"
#include "Athena/Core/PlatformUtils.h"

#include "Panels/Panel.h"

#include <array>


namespace Athena
{
	class ProfilingPanel : public Panel
	{
	public:
		ProfilingPanel(std::string_view name, const Ref<EditorContext>& context);

		virtual void OnImGuiRender() override;
	};
}
