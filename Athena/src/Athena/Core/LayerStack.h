#pragma once

#include "Core.h"
#include "Layer.h"


namespace Athena
{
	class ATHENA_API LayerStack
	{
	public:
		using iterator = std::vector<Layer*>::iterator;
		using const_iterator = std::vector<Layer*>::const_iterator;

	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		inline iterator		  begin()		{ return m_Layers.begin(); }
		inline const_iterator begin() const { return m_Layers.begin(); }
		inline iterator		  end()			{ return m_Layers.end();   }
		inline const_iterator end()   const { return m_Layers.end();   }

	private:
		std::vector<Layer*> m_Layers;
		uint32 m_LayerInsertIndex = 0;
	};
}
