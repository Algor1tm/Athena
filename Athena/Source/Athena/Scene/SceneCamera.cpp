#include "SceneCamera.h"

#include "Athena/Math/Projections.h"


namespace Athena
{
	SceneCamera::SceneCamera(ProjectionType type)
		: m_ProjectionType(type)
	{
		RecalculateProjection();
	}

	void SceneCamera::SetViewportSize(uint32 width, uint32 height)
	{
		m_AspecRatio = (float)width / (float)height;
		RecalculateProjection();
	}

	void SceneCamera::RecalculateProjection()
	{
		if (m_ProjectionType == ProjectionType::Perspective)
		{
			m_NearClip = m_PerspectiveData.NearClip;
			m_FarClip = m_PerspectiveData.FarClip;
		}
		else if (m_ProjectionType == ProjectionType::Orthographic)
		{
			m_NearClip = m_OrthoData.NearClip;
			m_FarClip = m_OrthoData.FarClip;
		}

		if (m_ProjectionType == ProjectionType::Orthographic)
		{
			float orthoLeft = -m_OrthoData.Size * m_AspecRatio * 0.5f;
			float orthoRight = m_OrthoData.Size * m_AspecRatio * 0.5f;
			float orthoBottom = -m_OrthoData.Size * 0.5f;
			float orthoTop = m_OrthoData.Size * 0.5f;

			m_ProjectionMatrix = Math::Ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthoData.NearClip, m_OrthoData.FarClip);
		}
		else if (m_ProjectionType == ProjectionType::Perspective)
		{
			m_ProjectionMatrix = Math::Perspective(m_PerspectiveData.VerticalFOV, m_AspecRatio,
				m_PerspectiveData.NearClip, m_PerspectiveData.FarClip);
		}
	}
}
