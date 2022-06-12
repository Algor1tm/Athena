#include "atnpch.h"
#include "OrthographicCamera.h"


namespace Athena 
{
	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
		: m_ProjectionMatrix(Ortho(left, right, bottom, top, -1.f, 1.f)), 
		  m_Position(0.f), m_Rotation(0.f)
	{
		m_ViewProjectionMatrix = m_ViewMatrix * m_ProjectionMatrix;
	}

	void OrthographicCamera::RecalculateViewMatrix()
	{
		Matrix4 transform = Rotate(m_Rotation, { 0, 0, 1 }) * Translate(m_Position);
		
		m_ViewMatrix = QuickInverse(transform);
		m_ViewProjectionMatrix = m_ViewMatrix * m_ProjectionMatrix;
	}
}
