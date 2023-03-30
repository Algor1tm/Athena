#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/UUID.h"

#include "Athena/Math/Transforms.h"
#include "Athena/Math/TypeCasts.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Mesh.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Texture.h"

#include "Athena/Scene/Entity.h"
#include "Athena/Scene/SceneCamera.h"

#include "Athena/Scripting/PublicScriptEngine.h"


namespace Athena
{
	struct IDComponent
	{
		UUID ID;
	};

	struct TagComponent
	{
		String Tag;

		TagComponent(const String& tag)
			: Tag(tag) {}
	};

	struct TransformComponent
	{
		Vector3 Translation = { 0.f, 0.f, 0.f };
		Vector3 Rotation = { 0.f, 0.f, 0.f }; 
		Vector3 Scale = { 1.f, 1.f, 1.f }; 

		TransformComponent() = default;
		TransformComponent(const Vector3& position)
			: Translation(position) {}

		Matrix4 AsMatrix() const
		{
			return Math::ToMat4(Math::ToQuat(Rotation)).Scale(Scale).Translate(Translation);
		}
	};

	struct ParentComponent
	{
		std::vector<Entity> Children;
	};

	struct ChildComponent
	{
		Entity Parent;
	};

	struct SpriteComponent
	{
		LinearColor Color;
		Texture2DInstance Texture;
		float TilingFactor;

		SpriteComponent(const LinearColor& color = LinearColor::White)
			: Color(color), Texture(Renderer::GetWhiteTexture()), TilingFactor(1.f) {}

		SpriteComponent(const Texture2DInstance& texture, const LinearColor& tint = LinearColor::White, float tilingFactor = 1.f)
			: Color(tint), Texture(texture), TilingFactor(tilingFactor) {}
	};

	struct CircleComponent
	{
		LinearColor Color;
		float Thickness = 1.f;
		float Fade = 0.005f;

		CircleComponent(const LinearColor& color = LinearColor::White)
			: Color(color) {}
	};

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;
	};

	struct ScriptComponent
	{
		std::string Name;
	};

	// Forward declaration (defined in NativeScript.h)
	class ATHENA_API NativeScript;

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


	struct Rigidbody2DComponent
	{
		enum class BodyType { STATIC = 0, DYNAMIC = 1, KINEMATIC = 2 };
		BodyType Type = BodyType::STATIC;
		bool FixedRotation = false;

		//Storage
		void* RuntimeBody = nullptr;
	};

	struct BoxCollider2DComponent
	{
		Vector2 Offset = { 0.f, 0.f };
		Vector2 Size = { 0.5f, 0.5f };

		float Density = 1.f;
		float Friction = 0.5f;
		float Restitution = 0.f;
		float RestitutionThreshold = 0.5f;

		//Storage
		void* RuntimeFixture = nullptr;
	};

	struct CircleCollider2DComponent
	{
		Vector2 Offset = { 0.f, 0.f };
		float Radius = 0.5f;

		float Density = 1.f;
		float Friction = 0.5f;
		float Restitution = 0.f;
		float RestitutionThreshold = 0.5f;

		//Storage
		void* RuntimeFixture = nullptr;
	};

	struct StaticMeshComponent
	{
		Ref<StaticMesh> Mesh;
		bool Hide = false;

		StaticMeshComponent() = default;

		StaticMeshComponent(const StaticMeshComponent& other)
		{
			Mesh = StaticMesh::Create(other.Mesh->GetFilePath());
			Hide = other.Hide;
		}

		StaticMeshComponent& operator=(const StaticMeshComponent& other)
		{
			Mesh = StaticMesh::Create(other.Mesh->GetFilePath());
			Hide = other.Hide;
			return *this;
		}
	};

	struct DirectionalLightComponent
	{
		LinearColor Color = LinearColor::White;
		float Intensity = 1;
	};

	struct PointLightComponent
	{
		LinearColor Color = LinearColor::White;
		float Intensity = 1.f;
		float Radius = 10.f;
		float FallOff = 1.f;
	};


	template<typename... Component>
	struct ComponentGroup
	{
	};

	using AllComponents =
		ComponentGroup<TransformComponent, ParentComponent, ChildComponent,
		SpriteComponent, CircleComponent, CameraComponent, 
		ScriptComponent, NativeScriptComponent, 
		Rigidbody2DComponent, BoxCollider2DComponent, CircleCollider2DComponent, 
		StaticMeshComponent, DirectionalLightComponent, PointLightComponent>;


	template <typename T>
	inline void Scene::OnComponentAdd(Entity entity, T& component) {}

	template<typename T>
	inline void Scene::OnComponentRemove(Entity entity, T& component) {}

	template <>
	inline void Scene::OnComponentAdd<CameraComponent>(Entity entity, CameraComponent& camera)
	{
		camera.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template <>
	inline void Scene::OnComponentRemove<ScriptComponent>(Entity entity, ScriptComponent& component)
	{
		PublicScriptEngine::OnScriptComponentRemove(entity);
	}
}
