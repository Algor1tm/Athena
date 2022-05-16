#pragma once

#include "Core.h"
#include "Layer.h"


namespace Athena
{
	class ATHENA_API LayerStack
	{
	public:
		using value_type = Layer*;
		using container = std::vector<value_type>;
		using iterator = container::iterator;
		using const_iterator = container::const_iterator;

	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer(value_type layer);
		void PushOverlay(value_type overlay);
		void PopLayer(value_type layer);
		void PopOverlay(value_type overlay);

		inline iterator		  begin()		{ return m_Layers.begin(); }
		inline const_iterator begin() const { return m_Layers.begin(); }
		inline iterator		  end()			{ return m_Layers.end();   }
		inline const_iterator end()   const { return m_Layers.end();   }

	private:
		container m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};
}
