#include "atnpch.h"
#include "OrthographicCamera.h"


namespace Athena 
{
	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
		: m_ProjectionMatrix(Math::Ortho(left, right, bottom, top, -1.f, 1.f)), m_ViewMatrix(Matrix4::Identity()),
		  m_Position(0.f), m_Rotation(0.f)
	{
		ATN_PROFILE_FUNCTION();

		m_ViewProjectionMatrix = m_ViewMatrix * m_ProjectionMatrix;
	}

	void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
	{
		ATN_PROFILE_FUNCTION();

		m_ProjectionMatrix = Math::Ortho(left, right, bottom, top, -1.f, 1.f);
		m_ViewProjectionMatrix = m_ViewMatrix * m_ProjectionMatrix;
	}

	void OrthographicCamera::RecalculateViewMatrix()
	{
		ATN_PROFILE_FUNCTION();

		Matrix4 transform = RotateMatrix(m_Rotation, Vector3(0, 0, 1)) * TranslateMatrix(m_Position);
		
		m_ViewMatrix = AffineInverse(transform);
		m_ViewProjectionMatrix = m_ViewMatrix * m_ProjectionMatrix;
	}
}
