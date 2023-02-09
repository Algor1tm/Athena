#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Panels/Panel.h"

#include <array>


namespace Athena
{
	class ProfilingPanel : public Panel
	{
	public:
		ProfilingPanel(std::string_view name);

		virtual void OnImGuiRender() override;

	private:
		bool m_IsPlottingFrameRate = false;

		std::array<float, 64> m_FrameRateStack;
		SIZE_T m_FrameRateIndex = 0;

		Time m_FrameTime;
		Timer m_Timer;
		Time m_LastTime = 0;

		const Time m_UpdateInterval = Time::Seconds(0.05f);
	};
}
