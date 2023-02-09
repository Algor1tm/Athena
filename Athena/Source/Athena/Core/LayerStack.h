#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class ATHENA_API Layer;


	class ATHENA_API LayerStack
	{
	public:
		using iterator = std::vector<Ref<Layer>>::iterator;
		using const_iterator = std::vector<Ref<Layer>>::const_iterator;

	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer(Ref<Layer> layer);
		void PushOverlay(Ref<Layer> overlay);
		void PopLayer(Ref<Layer> layer);
		void PopOverlay(Ref<Layer> overlay);

		inline iterator		  begin()		{ return m_Layers.begin(); }
		inline const_iterator begin() const { return m_Layers.begin(); }
		inline iterator		  end()			{ return m_Layers.end();   }
		inline const_iterator end()   const { return m_Layers.end();   }

	private:
		std::vector<Ref<Layer>> m_Layers;
		uint32 m_LayerInsertIndex = 0;
	};
}
