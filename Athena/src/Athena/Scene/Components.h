#pragma once

#include "Athena/Math/Matrix.h"
#include "Athena/Core/Color.h"
#include "Athena/Scene/SceneCamera.h"
#include "Athena/Renderer/Texture.h"

#include "NativeScript.h"

#include <functional>


namespace Athena
{
	struct TagComponent
	{
		String Tag;

		TagComponent(const String& tag)
			: Tag(tag) {}
	};

	struct TransformComponent
	{
		Vector3 Position = { 0.f, 0.f, 0.f };
		Vector3 Rotation = { 0.f, 0.f, 0.f };
		Vector3 Scale = { 1.f, 1.f, 1.f };

		TransformComponent() = default;
		TransformComponent(const Vector3& position)
			: Position(position) {}

		Matrix4 AsMatrix() const
		{
			Matrix4 transofrm = Math::ScaleMatrix(Scale) * Math::EulerAngles(Rotation.x, Rotation.y, Rotation.z);
			return transofrm.Translate(Position);
		}
	};

	struct SpriteComponent
	{
		LinearColor Color;
		Texture2DInstance Texture;
		float TilingFactor;

		SpriteComponent(const LinearColor& color = LinearColor::White)
			: Color(color), TilingFactor(1.f) {}

		SpriteComponent(const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f)
			: Color(tint), Texture(texture), TilingFactor(tilingFactor) {}
	};

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;
	};

	template <>
	inline void Scene::OnComponentAdd<CameraComponent>(Entity entity, CameraComponent& camera)
	{
		camera.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	struct NativeScriptComponent
	{
		NativeScript* Script = nullptr;

		NativeScript*(*InstantiateScript)() = nullptr;
		void (*DestroyScript)(NativeScriptComponent*) = nullptr;
		
		template <typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<NativeScript*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Script; nsc->Script = nullptr; };
		}
	};
}
