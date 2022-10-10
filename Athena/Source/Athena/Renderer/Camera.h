#pragma once

#include "Athena/Math/Matrix.h"


namespace Athena
{
	class ATHENA_API Camera
	{
	public:
		Camera(const Matrix4& projection = Matrix4::Identity())
			: m_Projection(projection) {}

		const Matrix4& GetProjection() const { return m_Projection; }
		void SetProjection(const Matrix4& projection) { m_Projection = projection; }

	protected:
		Matrix4 m_Projection;
	};
}
