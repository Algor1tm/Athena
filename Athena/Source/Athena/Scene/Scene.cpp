#include "Scene.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Environment.h"
#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/Renderer.h"

#include "Athena/Scene/Entity.h"
#include "Athena/Scene/Components.h"
#include "Athena/Scene/NativeScript.h"

#include "Athena/Scripting/ScriptEngine.h"

#ifdef _MSC_VER
	#pragma warning(push, 0)
#endif

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>

#ifdef _MSC_VER
	#pragma warning(pop)
#endif


namespace Athena
{
	template<typename... Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
			{
				auto view = src.view<Component>();
		for (auto srcEntity : view)
		{
			entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);

			auto& srcComponent = src.get<Component>(srcEntity);
			dst.emplace_or_replace<Component>(dstEntity, srcComponent);
		}
			}(), ...);
	}

	template<typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template<typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
			{
				if (src.HasComponent<Component>())
				dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
			}(), ...);
	}

	template<typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}


	static b2BodyType AthenaRigidBody2DTypeToBox2D(Rigidbody2DComponent::BodyType type)
	{
		switch (type)
		{
		case Rigidbody2DComponent::BodyType::STATIC: return b2BodyType::b2_staticBody;
		case Rigidbody2DComponent::BodyType::DYNAMIC: return b2BodyType::b2_dynamicBody;
		case Rigidbody2DComponent::BodyType::KINEMATIC: return b2BodyType::b2_kinematicBody;
		}
		ATN_CORE_ASSERT(false, "Undefined Rigidbody2DComponent BodyType!");
		return (b2BodyType)0;
	}

	Scene::Scene(const String& name)
		: m_Name(name)
	{
		m_Environment = CreateRef<Environment>();
	}

	Scene::~Scene()
	{

	}

	Ref<Scene> Scene::Copy(Ref<Scene> other)
	{
		Ref<Scene> newScene = CreateRef<Scene>();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		newScene->m_Environment = other->m_Environment;

		auto& srcSceneRegistry = other->m_Registry;
		auto& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;

		// Create entities in new scene
		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto entity : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(entity).ID;
			const auto& name = srcSceneRegistry.get<TagComponent>(entity).Tag;
			Entity newEntity = newScene->CreateEntity(name, uuid);
			enttMap[uuid] = (entt::entity)newEntity;
		}

		// Copy components (except IDComponent and TagComponent)
		CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);

		return newScene;
	}

	Entity Scene::CreateEntity(const String& name, UUID id)
	{
		Entity entity(m_Registry.create(), this);
		entity.AddComponent<IDComponent>(id);
		entity.AddComponent<TagComponent>(name);
		entity.AddComponent<TransformComponent>();

		m_EntityMap[id] = entity;

		return entity;
	}

	Entity Scene::CreateEntity(const String& name)
	{
		return CreateEntity(name, UUID());
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_EntityMap.erase(entity.GetID());
		m_Registry.destroy(entity);
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		// Copy name because we're going to modify component data structure
		String name = entity.GetName();
		Entity newEntity = CreateEntity(name);
		CopyComponentIfExists(AllComponents{}, newEntity, entity);
		return newEntity;
	}

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		ATN_CORE_ASSERT(m_EntityMap.find(uuid) != m_EntityMap.end());

		return { m_EntityMap.at(uuid), this };
	}

	Entity Scene::FindEntityByName(const String& name)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			if (view.get<TagComponent>(entity).Tag == name)
				return Entity{ entity, this };
		}

		return {};
	}

	void Scene::OnUpdateEditor(Time frameTime, const EditorCamera& camera)
	{
		// Update Animations
		auto view = m_Registry.view<StaticMeshComponent>();
		for (auto entity : view)
		{
			auto& meshComponent = view.get<StaticMeshComponent>(entity);
			if (meshComponent.Mesh->HasAnimations())
			{
				meshComponent.Mesh->GetAnimator()->OnUpdate(frameTime);
			}
		}

		RenderEditorScene(camera);
	}

	void Scene::OnUpdateRuntime(Time frameTime)
	{
		// Update scripts
		{
			auto view = m_Registry.view<ScriptComponent>();
			for (auto id : view)
			{
				Entity entity = { id, this };
				ScriptEngine::OnUpdateEntity(entity, frameTime);
			}

			m_Registry.view<NativeScriptComponent>().each([=](auto entityID, auto& nsc)
				{
					if (!nsc.Script)
					{
						nsc.Script = nsc.InstantiateScript();
						nsc.Script->m_Entity = Entity(entityID, this);

						nsc.Script->Init();
					}

					nsc.Script->OnUpdate(frameTime);
				});
		}

		// Update Animations
		auto view = m_Registry.view<StaticMeshComponent>();
		for (auto entity : view)
		{
			auto& meshComponent = view.get<StaticMeshComponent>(entity);
			if (meshComponent.Mesh->HasAnimations())
			{
				meshComponent.Mesh->GetAnimator()->OnUpdate(frameTime);
			}
		}

		// Physics
		UpdatePhysics(frameTime);

		// Choose camera
		Camera* mainCamera = nullptr;
		TransformComponent cameraTransform;
		{
			auto group = m_Registry.group<CameraComponent>(entt::get<TransformComponent>);
			for (auto entity : group)
			{
				auto [camera, transformComponent] = group.get<CameraComponent, TransformComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = transformComponent;
					break;
				}
			}
		}

		// Render 2D
		{
			if (mainCamera)
			{
				Matrix4 viewMatrix = Math::AffineInverse(cameraTransform.AsMatrix());
				Matrix4 projectionMatrix = (*mainCamera).GetProjectionMatrix();

				RenderRuntimeScene(viewMatrix, projectionMatrix);
			}
		}
	}

	void Scene::OnUpdateSimulation(Time frameTime, const EditorCamera& camera)
	{
		UpdatePhysics(frameTime);
		RenderEditorScene(camera);
	}

	void Scene::OnRuntimeStart()
	{
		OnPhysics2DStart();

		// Scripting
		{
			ScriptEngine::OnRuntimeStart(this);

			// Instantiate all script entities
			auto view = m_Registry.view<ScriptComponent>();
			for (auto id: view)
			{
				Entity entity = { id, this };
				ScriptEngine::InstantiateEntity(entity);
			}

			for (auto id: view)
			{
				Entity entity = { id, this };
				ScriptEngine::OnCreateEntity(entity);
			}
		}
	}

	void Scene::OnSimulationStart()
	{
		OnPhysics2DStart();
	}

	void Scene::OnViewportResize(uint32 width, uint32 height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
			{
				cameraComponent.Camera.SetViewportSize(width, height);
			}
		}
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (cameraComponent.Primary)
				return Entity(entity, this);
		}

		return Entity{};
	}

	void Scene::OnPhysics2DStart()
	{
		m_PhysicsWorld = std::make_unique<b2World>(b2Vec2(0, -9.8f));
		m_Registry.view<Rigidbody2DComponent, TransformComponent>().each([this](auto entityID, auto& rb2d, auto& transform)
			{
				Entity entity = Entity(entityID, this);

				b2BodyDef bodyDef;
				bodyDef.type = AthenaRigidBody2DTypeToBox2D(rb2d.Type);
				bodyDef.position.Set(transform.Translation.x, transform.Translation.y);
				bodyDef.angle = transform.Rotation.z;

				b2Body* body = m_PhysicsWorld->CreateBody(&bodyDef);
				body->SetFixedRotation(rb2d.FixedRotation);
				rb2d.RuntimeBody = reinterpret_cast<void*>(body);

				if (entity.HasComponent<BoxCollider2DComponent>())
				{
					auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

					b2PolygonShape boxShape;
					boxShape.SetAsBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y, b2Vec2(bc2d.Offset.y, bc2d.Offset.x), 0.f);

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &boxShape;
					fixtureDef.density = bc2d.Density;
					fixtureDef.friction = bc2d.Friction;
					fixtureDef.restitution = bc2d.Restitution;
					fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;
					body->CreateFixture(&fixtureDef);
				}

				if (entity.HasComponent<CircleCollider2DComponent>())
				{
					auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

					b2CircleShape circleShape;
					circleShape.m_p.Set(cc2d.Offset.x, cc2d.Offset.y);
					circleShape.m_radius = cc2d.Radius * transform.Scale.x;

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &circleShape;
					fixtureDef.density = cc2d.Density;
					fixtureDef.friction = cc2d.Friction;
					fixtureDef.restitution = cc2d.Restitution;
					fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;
					body->CreateFixture(&fixtureDef);
				}
			});
	}

	void Scene::UpdatePhysics(Time frameTime)
	{
		constexpr uint32 velocityIterations = 6;
		constexpr uint32 positionIterations = 2;
		m_PhysicsWorld->Step(frameTime.AsSeconds(), velocityIterations, positionIterations);

		m_Registry.view<Rigidbody2DComponent, TransformComponent>().each([](auto entityID, auto& rb2d, auto& transform)
			{
				b2Body* body = reinterpret_cast<b2Body*>(rb2d.RuntimeBody);
				const auto& position = body->GetPosition();
				transform.Translation.x = position.x;
				transform.Translation.y = position.y;
				transform.Rotation.z = body->GetAngle();
			});
	}

	void Scene::RenderEditorScene(const EditorCamera& camera)
	{
		Matrix4 viewMatrix = camera.GetViewMatrix();
		Matrix4 projectionMatrix = camera.GetProjectionMatrix();

		Renderer2D::BeginScene(viewMatrix, projectionMatrix);

		auto quads = GetAllEntitiesWith<TransformComponent, SpriteComponent>();
		for (auto entity : quads)
		{
			auto [transform, sprite] = quads.get<TransformComponent, SpriteComponent>(entity);

			Renderer2D::DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Color, sprite.TilingFactor, (int32)entity);
		}

		auto circles = GetAllEntitiesWith<TransformComponent, CircleComponent>();
		for (auto entity : circles)
		{
			auto [transform, circle] = circles.get<TransformComponent, CircleComponent>(entity);

			Renderer2D::DrawCircle(transform.AsMatrix(), circle.Color, circle.Thickness, circle.Fade, (int32)entity);
		}

		Renderer2D::EndScene();


		Renderer::BeginScene(viewMatrix, projectionMatrix, m_Environment);

		auto staticMeshes = GetAllEntitiesWith<TransformComponent, StaticMeshComponent>();
		for (auto entity : staticMeshes)
		{
			auto [transform, meshComponent] = staticMeshes.get<TransformComponent, StaticMeshComponent>(entity);

			if (!meshComponent.Hide)
			{
				const auto& subMeshes = meshComponent.Mesh->GetAllSubMeshes();
				for (uint32 i = 0; i < subMeshes.size(); ++i)
				{
					Ref<Material> material = MaterialManager::GetMaterial(subMeshes[i].MaterialName);
					Ref<Animator> animator = meshComponent.Mesh->GetAnimator();

					if (animator != nullptr && animator->IsPlaying())
					{
						Renderer::SubmitWithAnimation(subMeshes[i].VertexBuffer, material, animator->IsPlaying() ? animator : nullptr, transform.AsMatrix(), (int32)entity);
					}
					else
					{
						Renderer::Submit(subMeshes[i].VertexBuffer, material, transform.AsMatrix(), (int32)entity);
					}
				}
			}
		}

		auto dirLights = GetAllEntitiesWith<DirectionalLightComponent>();
		for (auto entity : dirLights)
		{
			auto& light = dirLights.get<DirectionalLightComponent>(entity);
			DirectionalLight dirLight = { light.Color, light.Direction, light.Intensity };
			Renderer::SubmitLight(dirLight);
		}

		auto pointLights = GetAllEntitiesWith<TransformComponent, PointLightComponent>();
		for (auto entity : pointLights)
		{
			auto [transform, light] = pointLights.get<TransformComponent, PointLightComponent>(entity);
			PointLight pointLight = { light.Color, transform.Translation, light.Intensity, light.Radius, light.FallOff };
			Renderer::SubmitLight(pointLight);
		}

		Renderer::EndScene();
	}

	void Scene::RenderRuntimeScene(const Matrix4& viewMatrix, const Matrix4& projectionMatrix)
	{
		Renderer2D::BeginScene(viewMatrix, projectionMatrix);

		auto quads = GetAllEntitiesWith<TransformComponent, SpriteComponent>();
		for (auto entity : quads)
		{
			auto [transform, sprite] = quads.get<TransformComponent, SpriteComponent>(entity);

			Renderer2D::DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Color, sprite.TilingFactor);
		}

		auto circles = GetAllEntitiesWith<TransformComponent, CircleComponent>();
		for (auto entity : circles)
		{
			auto [transform, circle] = circles.get<TransformComponent, CircleComponent>(entity);

			Renderer2D::DrawCircle(transform.AsMatrix(), circle.Color, circle.Thickness, circle.Fade);
		}

		Renderer2D::EndScene();


		Renderer::BeginScene(viewMatrix, projectionMatrix, m_Environment);

		auto staticMeshes = GetAllEntitiesWith<TransformComponent, StaticMeshComponent>();
		for (auto entity : staticMeshes)
		{
			auto [transform, meshComponent] = staticMeshes.get<TransformComponent, StaticMeshComponent>(entity);

			if (!meshComponent.Hide)
			{
				const auto& subMeshes = meshComponent.Mesh->GetAllSubMeshes();
				for (uint32 i = 0; i < subMeshes.size(); ++i)
				{
					Ref<Material> material = MaterialManager::GetMaterial(subMeshes[i].MaterialName);
					Ref<Animator> animator = meshComponent.Mesh->GetAnimator();

					if (animator != nullptr && animator->IsPlaying())
					{
						Renderer::SubmitWithAnimation(subMeshes[i].VertexBuffer, material, animator->IsPlaying() ? animator : nullptr, transform.AsMatrix(), (int32)entity);
					}
					else
					{
						Renderer::Submit(subMeshes[i].VertexBuffer, material, transform.AsMatrix(), (int32)entity);
					}
				}
			}
		}

		auto dirLights = GetAllEntitiesWith<DirectionalLightComponent>();
		for (auto entity : dirLights)
		{
			auto& light = dirLights.get<DirectionalLightComponent>(entity);
			DirectionalLight dirLight = { light.Color, light.Direction, light.Intensity };
			Renderer::SubmitLight(dirLight);
		}

		auto pointLights = GetAllEntitiesWith<TransformComponent, PointLightComponent>();
		for (auto entity : pointLights)
		{
			auto [transform, light] = pointLights.get<TransformComponent, PointLightComponent>(entity);
			PointLight pointLight = { light.Color, transform.Translation, light.Intensity, light.Radius, light.FallOff };
			Renderer::SubmitLight(pointLight);
		}

		Renderer::EndScene();
	}
}
