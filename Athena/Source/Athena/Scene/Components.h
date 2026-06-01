#pragma once

#include "Athena/Asset/AssetManager.h"
#include "Athena/Core/Core.h"
#include "Athena/Core/UUID.h"
#include "Athena/Math/Transforms.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/EnvironmentMap.h"
#include "Athena/Renderer/Mesh.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/EngineTextures.h"
#include "Athena/Scene/Entity.h"
#include "Athena/Scene/SceneCamera.h"
#include "Athena/Scripting/ScriptEngine.h"


namespace Athena
{
	class ATHENA_API Scene;

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

	struct WorldTransformComponent
	{
		Vector3 Translation = { 0.f, 0.f, 0.f };
		Quaternion Rotation = { 1.f, 0.f, 0.f, 0.f };
		Vector3 Scale = { 1.f, 1.f, 1.f };

		WorldTransformComponent() = default;
		WorldTransformComponent(const Vector3& position)
			: Translation(position) {}

		Matrix4 AsMatrix() const
		{
			return Math::ConstructTransform(Translation, Scale, Rotation);
		}
	};

	struct TransformComponent
	{
		Vector3 Translation = { 0.f, 0.f, 0.f };
		Quaternion Rotation = { 1.f, 0.f, 0.f, 0.f };
		Vector3 Scale = { 1.f, 1.f, 1.f };

		TransformComponent() = default;
		TransformComponent(const Vector3& position)
			: Translation(position) {}

		Matrix4 AsMatrix() const
		{
			return Math::ConstructTransform(Translation, Scale, Rotation);
		}

		TransformComponent& UpdateLocalTransform(const WorldTransformComponent& newWorldTransform, const WorldTransformComponent& oldWorldTransform)
		{
			TransformComponent parentTransform;
			parentTransform.Rotation = oldWorldTransform.Rotation * Rotation.GetInversed();
			parentTransform.Translation = -(parentTransform.Rotation * Translation) + oldWorldTransform.Translation;
			parentTransform.Scale = (Vector3(1.f) / Scale) * oldWorldTransform.Scale;

			Translation = parentTransform.Rotation.GetInversed() * (-parentTransform.Translation + newWorldTransform.Translation);
			Rotation = parentTransform.Rotation.GetInversed() * newWorldTransform.Rotation;
			Scale = (Vector3(1.f) / parentTransform.Scale) * newWorldTransform.Scale;

			return *this;
		}
	};

	struct ChildComponent
	{
		std::vector<Entity> Children;
	};

	struct ParentComponent
	{
		Entity Parent;
	};

	struct SpriteComponent
	{
		Renderer2DSpace Space = Renderer2DSpace::WorldSpace;
		LinearColor Color = LinearColor::White;
		AssetHandle TextureHandle = 0;
		float TilingFactor = 1.f;
	};

	struct CircleComponent
	{
		Renderer2DSpace Space = Renderer2DSpace::WorldSpace;
		LinearColor Color;
		float Thickness = 1.f;
		float Fade = 0.005f;

		CircleComponent(const LinearColor& color = LinearColor::White)
			: Color(color) {}
	};

	struct TextComponent
	{
		String Text;
		AssetHandle FontHandle = 0;
		Renderer2DSpace Space = Renderer2DSpace::WorldSpace;
		LinearColor Color = LinearColor::White;
		float MaxWidth = 10.f;
		float Kerning = 0.0f;
		float LineSpacing = 0.0f;
		bool Shadowing = false;
		float ShadowDistance = 1.f;
		LinearColor ShadowColor = LinearColor::Black;
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

	struct ATHENA_API MeshComponent
	{
		AssetHandle MeshHandle = 0;
		MaterialTable OverrideMaterials;
		uint32 MeshNodeIndex = 0;
		bool Visible = true;

		void ResetMaterials();
		Ref<Material> GetMaterial(const Ref<Mesh>& mesh, const String& materialName) const;

		bool IsRootMeshNode() const { return MeshNodeIndex == 0; }
	};

	struct AnimationControllerComponent
	{
		Ref<AnimationController> AnimationController;
	};

	struct DirectionalLightComponent
	{
		LinearColor Color = LinearColor::White;
		float Intensity = 1.f;
		bool CastShadows = true;
		float LightSize = 0.4f;
	};

	struct PointLightComponent
	{
		LinearColor Color = LinearColor::White;
		float Intensity = 1.f;
		float Radius = 10.f;
		float FallOff = 1.f;
	};

	struct SpotLightComponent
	{
		LinearColor Color = LinearColor::White;
		float Intensity = 1.f;
		float SpotAngle = 30.f;
		float InnerFallOff = 1.f;
		float Range = 10.f;
		float RangeFallOff = 1.f;
	};

	struct SkyLightComponent
	{
		EnvironmentMapType Type = EnvironmentMapType::PREETHAM;
		AssetHandle EnvMapHandle = 0;
		PreethamParams Preetham;

		float LOD = 0.f;
		float Intensity = 1.f;
	};


	template<typename... Component>
	struct ComponentGroup
	{
	};

	using AllComponents =
		ComponentGroup<WorldTransformComponent, TransformComponent, ParentComponent, ChildComponent,
		SpriteComponent, CircleComponent, TextComponent, 
		ScriptComponent, CameraComponent,
		Rigidbody2DComponent, BoxCollider2DComponent, CircleCollider2DComponent, 
		MeshComponent, AnimationController,
		DirectionalLightComponent, PointLightComponent, SpotLightComponent, SkyLightComponent>;


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
	inline void Scene::OnComponentRemove<ScriptComponent>(Entity entity, ScriptComponent& script)
	{
		ScriptEngine::OnEntityScriptRemove(entity);
	}
}
