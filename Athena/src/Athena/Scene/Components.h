#pragma once

#include "Athena/Math/Matrix.h"
#include "Athena/Core/Color.h"
#include "Athena/Scene/SceneCamera.h"
#include "NativeScript.h"

#include <functional>


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

	struct NativeScriptComponent
	{
		NativeScript* Script = nullptr;

		NativeScript*(*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);
		
		template <typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<NativeScript*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Script; nsc->Script = nullptr; };
		}
	};
}
