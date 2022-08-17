#include "atnpch.h"
#include "SceneCamera.h"

#include "Athena/Math/Utils/MatrixProjection.h"


namespace Athena
{
	SceneCamera::SceneCamera()
	{
		RecalculateProjection();
	}

	void SceneCamera::SetOrthographic(float size, float nearClip, float farClip)
	{
		m_OrthographicSize = size;
		m_OrthographicNear = nearClip;
		m_OrthographicFar = farClip;
	}

	void SceneCamera::SetViewportSize(uint32 width, uint32 height)
	{
		m_AspecRatio = (float)width / (float)height;
		RecalculateProjection();
	}

	void SceneCamera::RecalculateProjection()
	{
		float orthoLeft = -m_OrthographicSize * m_AspecRatio * 0.5f;
		float orthoRight = m_OrthographicSize * m_AspecRatio * 0.5f;
		float orthoBottom = -m_OrthographicSize * 0.5f;
		float orthoTop = m_OrthographicSize * 0.5f;

		SetProjection(Ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, m_OrthographicNear, m_OrthographicFar));
	}

}
