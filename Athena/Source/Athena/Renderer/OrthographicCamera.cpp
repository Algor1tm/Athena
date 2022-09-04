#include "atnpch.h"
#include "OrthographicCamera.h"


namespace Athena 
{
	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
		: m_ProjectionMatrix(Math::Ortho(left, right, bottom, top, -1.f, 1.f)), m_ViewMatrix(Matrix4::Identity()),
		  m_Position(0.f), m_Rotation(0.f)
	{
		m_ViewProjectionMatrix = m_ViewMatrix * m_ProjectionMatrix;
	}

	void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
	{
		m_ProjectionMatrix = Math::Ortho(left, right, bottom, top, -1.f, 1.f);
		m_ViewProjectionMatrix = m_ViewMatrix * m_ProjectionMatrix;
	}

	void OrthographicCamera::RecalculateView()
	{
		Matrix4 transform = Math::RotateMatrix(m_Rotation, Vector3(0, 0, 1)) * Math::TranslateMatrix(m_Position);
		
		m_ViewMatrix = Math::AffineInverse(transform);
		m_ViewProjectionMatrix = m_ViewMatrix * m_ProjectionMatrix;
	}
}
