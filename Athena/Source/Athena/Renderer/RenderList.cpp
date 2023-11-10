#include "RenderList.h"

#include "Athena/Renderer/Material.h"

#include "Athena/Math/Common.h"


namespace Athena
{
	void RenderList::Push(const DrawCallInfo& info)
	{
		m_Queue.push_back(info);
	}

	const DrawCallInfo& RenderList::Next()
	{
		m_CurrentIndex++;
		return m_Queue[m_CurrentIndex - 1];
	}

	void RenderList::Clear()
	{
		m_LastMaterial = nullptr;
		m_LastAnimator = nullptr;
		m_CurrentIndex = 0;
		m_Queue.clear();
	}

	void RenderList::Sort()
	{
		//std::sort(m_Queue.begin(), m_Queue.end(), [](const DrawCallInfo& left, const DrawCallInfo& right)
		//	{
		//		if (left.Animator != right.Animator)
		//			return left.Animator.Raw() < right.Animator.Raw();

		//		return left.Material->GetName() < right.Material->GetName();
		//	});
	}

	bool RenderList::HasStaticMeshes() const
	{
		return CheckLimit() && m_Queue[m_CurrentIndex].Animator == nullptr;
	}

	bool RenderList::HasAnimMeshes() const
	{
		return CheckLimit() && m_Queue[m_CurrentIndex].Animator != nullptr;
	}

	bool RenderList::UpdateMaterial()
	{
		if (m_Queue[m_CurrentIndex - 1].Material != m_LastMaterial || m_LastMaterial == nullptr)
		{
			m_LastMaterial = m_Queue[m_CurrentIndex - 1].Material;
			return true;
		}

		return false;
	}

	bool RenderList::UpdateAnimator()
	{
		if (m_Queue[m_CurrentIndex - 1].Animator != m_LastAnimator || m_LastAnimator == nullptr)
		{
			m_LastAnimator = m_Queue[m_CurrentIndex - 1].Animator;
			return true;
		}

		return false;
	}

	bool RenderList::CheckLimit() const
	{
		return m_CurrentIndex < m_Queue.size();
	}
}
