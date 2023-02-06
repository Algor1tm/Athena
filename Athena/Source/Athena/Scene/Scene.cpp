#include "Scene.h"

#include "Entity.h"
#include "Components.h"
#include "NativeScript.h"

#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Scene/SceneRenderer.h"
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

	void Scene::OnUpdateEditor(Time frameTime, EditorCamera& camera)
	{
		// Update Animations
		auto view = m_Registry.view<SkeletalMeshComponent>();
		for(auto entity : view)
		{
			auto meshComponent = view.get<SkeletalMeshComponent>(entity);

			if (meshComponent.Animator->IsPlaying())
			{
				meshComponent.Animator->OnUpdate(frameTime);
			}
		}

		SceneRenderer::RenderEditorScene(this, camera);
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
		auto view = m_Registry.view<SkeletalMeshComponent>();
		for (auto entity : view)
		{
			auto meshComponent = view.get<SkeletalMeshComponent>(entity);

			if (meshComponent.Animator->IsPlaying())
			{
				meshComponent.Animator->OnUpdate(frameTime);
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
				SceneRenderer::Render(this, viewMatrix, projectionMatrix);
			}
		}
	}

	void Scene::OnUpdateSimulation(Time frameTime, EditorCamera& camera)
	{
		UpdatePhysics(frameTime);
		SceneRenderer::RenderEditorScene(this, camera);
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
}
