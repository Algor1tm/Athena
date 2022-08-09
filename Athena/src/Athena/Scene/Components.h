#pragma once

#include "Athena/Math/Matrix.h"
#include "Athena/Renderer/Color.h"


namespace Athena
{
	struct TransformComponent
	{
		Matrix4 Transform;

		TransformComponent(const Matrix4& transform)
			: Transform(transform) {}

		operator Matrix4& () { return Transform; }
		operator const Matrix4& () const { return Transform; }
	};
}
