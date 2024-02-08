#include "DrawList.h"

#include "Athena/Renderer/Material.h"

#include "Athena/Math/Common.h"


namespace Athena
{
	void DrawList::Push(const DrawCall& drawCall)
	{
		m_Array.push_back(drawCall);
	}

	void DrawList::Clear()
	{
		m_LastMaterial = nullptr;
		m_Array.clear();
	}

	void DrawList::Sort()
	{
		std::sort(m_Array.begin(), m_Array.end(), [](const DrawCall& left, const DrawCall& right)
		{
			return left.Material->GetName() < right.Material->GetName();
		});
	}

	bool DrawList::UpdateMaterial(const DrawCall& drawCall)
	{
		if (drawCall.Material != m_LastMaterial)
		{
			m_LastMaterial = drawCall.Material;
			return true;
		}

		return false;
	}

	std::vector<DrawCall>::const_iterator DrawList::begin()
	{
		m_LastMaterial = nullptr;
		return m_Array.begin();
	}

	std::vector<DrawCall>::const_iterator DrawList::end()
	{
		return m_Array.end();
	}
}
