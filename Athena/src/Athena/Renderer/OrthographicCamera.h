#pragma once

#include "Athena/Math/Matrix.h"
#include "Athena/Math/Vector.h"


namespace Athena
{
	class OrthographicCamera
	{
	public:
		OrthographicCamera(float left, float right, float bottom, float top);

		inline const Matrix4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		inline const Matrix4& GetViewMatrix() const { return m_ViewMatrix; }
		inline const Matrix4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

		inline const Vector3& GetPosition() const { return m_Position; }
		inline float GetRotation() const { return m_Rotation; }

		inline void SetPosition(const Vector3& position) { m_Position = position; RecalculateViewMatrix(); }
		inline void SetRotation(float rotation) { m_Rotation = rotation; RecalculateViewMatrix(); }

	private:
		void RecalculateViewMatrix();

	private:
		Matrix4 m_ProjectionMatrix;
		Matrix4 m_ViewMatrix;
		Matrix4 m_ViewProjectionMatrix;

		Vector3 m_Position;
		float m_Rotation;
	};
}
