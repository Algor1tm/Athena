#pragma once

#include "Athena/Math/Matrix.h"


namespace Athena
{
	class ATHENA_API Camera
	{
	public:
		const Matrix4& GetProjectionMatrix() const { return m_ProjectionMatrix; }

	protected:
		Matrix4 m_ProjectionMatrix = Matrix4::Identity();
	};
}
