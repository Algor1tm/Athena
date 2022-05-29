#pragma once

#include "Core.h"
#include "Athena/Events/Event.h"


namespace Athena
{
	class ATHENA_API Layer
	{
	public:
		Layer(const std::string& name = "Layer")
			: m_Name(name) {}

		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate() {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		inline const std::string& GetName() const { return m_Name; }

	protected:
		std::string m_Name;
	};
}
