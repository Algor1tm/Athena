#include "RenderQueue.h"

#include "Athena/Renderer/Material.h"

#include "Athena/Math/Common.h"


namespace Athena
{
	void RenderQueue::Push(const DrawCallInfo& info)
	{
		m_Queue.push_back(info);
	}

	const DrawCallInfo& RenderQueue::Next()
	{
		m_CurrentIndex++;
		return m_Queue[m_CurrentIndex - 1];
	}

	void RenderQueue::Clear()
	{
		if (m_LastSize != m_Queue.size())
			m_Limit = m_Queue.size();

		m_LastSize = m_Queue.size();
		m_LastMaterial = nullptr;
		m_LastAnimator = nullptr;
		m_CurrentIndex = 0;
		m_Queue.clear();
	}

	void RenderQueue::Sort()
	{
		std::sort(m_Queue.begin(), m_Queue.end(), [](const DrawCallInfo& left, const DrawCallInfo& right)
			{
				if (left.Animator != right.Animator)
					return left.Animator < right.Animator;

				return left.Material->GetName() < right.Material->GetName();
			});
	}

	bool RenderQueue::HasStaticMeshes() const
	{
		return CheckLimit() && m_Queue[m_CurrentIndex].Animator == nullptr;
	}

	bool RenderQueue::HasAnimMeshes() const
	{
		return CheckLimit();
	}

	bool RenderQueue::UpdateMaterial()
	{
		if (m_Queue[m_CurrentIndex - 1].Material != m_LastMaterial || m_LastMaterial == nullptr)
		{
			m_LastMaterial = m_Queue[m_CurrentIndex - 1].Material;
			return true;
		}

		return false;
	}

	bool RenderQueue::UpdateAnimator()
	{
		if (m_Queue[m_CurrentIndex - 1].Animator != m_LastAnimator || m_LastAnimator == nullptr)
		{
			m_LastAnimator = m_Queue[m_CurrentIndex - 1].Animator;
			return true;
		}

		return false;
	}

	bool RenderQueue::CheckLimit() const
	{
		if (m_LastSize != m_Queue.size() || m_Limit < 0)
			return m_CurrentIndex < m_Queue.size();
		
		uint32 size = Math::Min((uint32)m_Limit, (uint32)m_Queue.size());
		return m_CurrentIndex < size;
	}
}
