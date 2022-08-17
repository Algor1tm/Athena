#pragma once

#include "Athena/Math/Matrix.h"


namespace Athena
{
	class Camera
	{
	public:
		Camera() = default;
		Camera(const Matrix4& projection)
			: m_Projection(projection) {}

		const Matrix4& GetProjection() const { return m_Projection; }
		void SetProjection(const Matrix4& projection) { m_Projection = projection; }

	private:
		Matrix4 m_Projection = Matrix4::Identity();
	};
}
