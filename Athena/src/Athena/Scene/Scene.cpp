#include "atnpch.h"

#include "Athena/Renderer/Renderer2D.h" 

#include "Scene.h"
#include "Entity.h"

#include "Components.h"
#include "NativeScript.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>


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

	Scene::Scene()
	{

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

		return entity;
	}

	Entity Scene::CreateEntity(const String& name)
	{
		return CreateEntity(name, UUID());
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void Scene::OnUpdateEditor(Time frameTime, EditorCamera& camera)
	{
		// Render 2D
		Renderer2D::BeginScene(camera);

		auto group = m_Registry.group<TransformComponent>(entt::get<SpriteComponent>);
		for (auto entity : group)
		{
			auto [transform, sprite] = group.get<TransformComponent, SpriteComponent>(entity);

			Renderer2D::DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Color, sprite.TilingFactor, (int)entity);
		}

		Renderer2D::EndScene();
	}

	void Scene::OnUpdateRuntime(Time frameTime)
	{
		//Run native scripts
		{
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

		//Physics
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

		// Choose camera
		Camera* mainCamera = nullptr;
		Matrix4 cameraTransform;
		{
			auto group = m_Registry.group<CameraComponent>(entt::get<TransformComponent>);
			for (auto entity : group)
			{
				auto [camera, transformComponent] = group.get<CameraComponent, TransformComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = transformComponent.AsMatrix();
					break;
				}
			}
		}

		// Render 2D
		{
			if (mainCamera)
			{
				Renderer2D::BeginScene(*mainCamera, cameraTransform);

				auto group = m_Registry.group<TransformComponent>(entt::get<SpriteComponent>);
				for (auto entity : group)
				{
					auto [transform, sprite] = group.get<TransformComponent, SpriteComponent>(entity);

					Renderer2D::DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Color, sprite.TilingFactor);
				}

				Renderer2D::EndScene();
			}
		}
	}

	void Scene::OnRuntimeStart()
	{
		m_PhysicsWorld = std::make_unique<b2World>(b2Vec2(0, -9.8f));
		m_Registry.view<Rigidbody2DComponent, TransformComponent>().each([=](auto entityID, auto& rb2d, auto& transform)
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
					boxShape.SetAsBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y);

					b2FixtureDef fixtureDef;
					fixtureDef.shape = &boxShape;
					fixtureDef.density = bc2d.Density;
					fixtureDef.friction = bc2d.Friction;
					fixtureDef.restitution = bc2d.Restitution;
					fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;
					body->CreateFixture(&fixtureDef);
				}
			});
	}

	void Scene::OnRuntimeStop()
	{
		
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
}
