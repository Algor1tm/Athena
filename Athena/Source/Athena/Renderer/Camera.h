#pragma once

#include "Athena/Math/Matrix.h"


namespace Athena
{
	class ATHENA_API Camera
	{
	public:
		Camera(const Matrix4& projection = Matrix4::Identity())
			: m_ProjectionMatrix(projection) {}

		const Matrix4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		void SetProjection(const Matrix4& projection) { m_ProjectionMatrix = projection; }

	protected:
		Matrix4 m_ProjectionMatrix;
	};
}
