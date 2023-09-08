#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Scene/Entity.h"


namespace Athena
{
	// In future extend this class for multiple selection
	class SelectionManager
	{
	public:
		static void Init() {}

		static void SelectEntity(Entity entity)
		{
			m_SelectedEntity = entity;
		}

		static void DeselectEntity()
		{
			m_SelectedEntity = {};
		}

		static Entity GetSelectedEntity() 
		{
			return m_SelectedEntity;
		}

	private:
		static inline Entity m_SelectedEntity;
	};
}