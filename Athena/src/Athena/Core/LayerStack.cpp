#include "atnpch.h"
#include "LayerStack.h"

#include "Log.h"


namespace Athena
{
	LayerStack::~LayerStack()
	{
		for (value_type layer : m_Layers)
		{
			layer->OnDetach();
			delete layer;
		}
	}


	void LayerStack::PushLayer(value_type layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
		layer->OnAttach();
		++m_LayerInsertIndex;
	}


	void LayerStack::PushOverlay(value_type overlay)
	{
		m_Layers.emplace_back(overlay);
		overlay->OnAttach();
	}


	void LayerStack::PopLayer(value_type layer)
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
			ATN_CORE_WARN("Could not pop layer with name = {0}", layer->GetName());
		}
	}


	void LayerStack::PopOverlay(value_type overlay)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (it != m_Layers.end())
		{
			overlay->OnDetach();
			m_Layers.erase(it);
		}
		else
		{
			ATN_CORE_WARN("Could not pop overlay with name = {0}", overlay->GetName());
		}
	}
}
