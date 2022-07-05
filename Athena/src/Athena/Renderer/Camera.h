#pragma once

#include "Athena/Math/Matrix.h"


namespace Athena
{
	class Camera
	{
	public:
		virtual const Matrix4& GetViewProjectionMatrix() const = 0;
	};
}
