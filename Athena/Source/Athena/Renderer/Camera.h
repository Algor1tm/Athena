#pragma once

#include "Athena/Math/Matrix.h"


namespace Athena
{
	struct CameraInfo
	{
		Matrix4 ProjectionMatrix;
		Matrix4 ViewMatrix;
		float NearClip;
		float FarClip;
	};

	class ATHENA_API Camera
	{
	public:
		Camera() = default;
		Camera(float nearClip, float farClip): m_NearClip(nearClip), m_FarClip(farClip) {}

		const Matrix4& GetProjectionMatrix() const { return m_ProjectionMatrix; }

		float GetNearClip() const { return m_NearClip; }
		float GetFarClip() const { return m_FarClip; }

	protected:
		Matrix4 m_ProjectionMatrix = Matrix4::Identity();
		float m_NearClip = 0.1f;
		float m_FarClip = 1000.f;
	};
}
