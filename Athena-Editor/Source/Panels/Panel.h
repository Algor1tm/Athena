#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class Panel
	{
	public:
		Panel(std::string_view name = "UnNamed")
			: m_Name(name) {}

		virtual void OnImGuiRender() = 0;

		std::string_view GetName() { return m_Name; }

	protected:
		std::string_view m_Name;
	};
}
