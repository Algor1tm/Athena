#include "LayerStack.h"

#include "Athena/Core/Layer.h"
#include "Athena/Core/Log.h"


namespace Athena
{
	LayerStack::~LayerStack()
	{
		Clear();
	}


	void LayerStack::PushLayer(Ref<Layer> layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
		++m_LayerInsertIndex;
	}


	void LayerStack::PushOverlay(Ref<Layer> overlay)
	{
		m_Layers.emplace_back(overlay);
	}


	void LayerStack::PopLayer(Ref<Layer> layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it != m_Layers.begin() + m_LayerInsertIndex)
		{
			layer->OnDetach();
			m_Layers.erase(it);
			--m_LayerInsertIndex;
		}
		else
		{
			ATN_CORE_ERROR_TAG("LayerStack", "Could not pop layer with name = {0}", layer->GetName());
		}
	}


	void LayerStack::PopOverlay(Ref<Layer> overlay)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (it != m_Layers.end())
		{
			overlay->OnDetach();
			m_Layers.erase(it);
		}
		else
		{
			ATN_CORE_ERROR_TAG("LayerStack", "Could not pop overlay with name = {0}", overlay->GetName());
		}
	}

	void LayerStack::Clear()
	{
		for (Ref<Layer> layer : m_Layers)
		{
			layer->OnDetach();
		}

		m_Layers.clear();
	}
}
