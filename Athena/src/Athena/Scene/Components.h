#pragma once

#include "Athena/Math/Matrix.h"
#include "Athena/Core/Color.h"
#include "Athena/Scene/SceneCamera.h"


namespace Athena
{
	struct TagComponent
	{
		std::string_view Tag;

		TagComponent(std::string_view tag)
			: Tag(tag) {}
	};

	struct TransformComponent
	{
		Matrix4 Transform;

		TransformComponent(const Matrix4& transform = Matrix4::Identity())
			: Transform(transform) {}

		operator Matrix4& () { return Transform; }
		operator const Matrix4& () const { return Transform; }
	};

	struct SpriteRendererComponent
	{
		LinearColor Color;

		SpriteRendererComponent(const LinearColor& color = LinearColor::White)
			: Color(color) {}
	};

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;
	};
}
