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
		ProfilingPanel(const Ref<EditorContext>& context);

		//void SetContext(Ref<SceneRenderer> sceneRenderer) { m_SceneRenderer = sceneRenderer; }
		virtual void OnImGuiRender() override;

	private:
		uint64 m_VRAMUsage = 0;
		//Ref<SceneRenderer> m_SceneRenderer;
	};
}
