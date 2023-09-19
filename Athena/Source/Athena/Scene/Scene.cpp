#include "Scene.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Environment.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/SceneRenderer2D.h"
#include "Athena/Renderer/SceneRenderer.h"

#include "Athena/Scene/Entity.h"
#include "Athena/Scene/Components.h"
#include "Athena/Scene/NativeScript.h"

#include "Athena/Scripting/PrivateScriptEngine.h"

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
					UUID id = src.get<IDComponent>(srcEntity).ID;
					entt::entity dstEntity = enttMap.at(id);

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

	static void DeleteFromChildren(std::vector<Entity>& children, Entity parent, Entity child)
	{
		auto iter = std::find(children.begin(), children.end(), child);
		if (iter != children.end())
		{
			children.erase(iter);
			if (children.size() == 0)
				parent.RemoveComponent<ParentComponent>();

		}
		else
		{
			const String& childTag = child.GetComponent<TagComponent>().Tag;
			const String& parentTag = parent.GetComponent<TagComponent>().Tag;
			ATN_CORE_WARN("Attempt to delete entity '{}' from children of entity '{}'", childTag, parentTag);
		}
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

		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto entity : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(entity).ID;
			const auto& name = srcSceneRegistry.get<TagComponent>(entity).Tag;
			Entity newEntity = newScene->CreateEntity(name, uuid);
		}

		CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, newScene->m_EntityMap);

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

	Entity Scene::CreateEntity(const String& name, UUID id, Entity parent)
	{
		Entity entity = CreateEntity(name, id);
		MakeRelationship(parent, entity);

		return entity;
	}

	Entity Scene::CreateEntity(const String& name)
	{
		return CreateEntity(name, UUID());
	}

	void Scene::DestroyEntity(Entity entity)
	{
		if (entity.HasComponent<ParentComponent>())
		{
			auto& children = entity.GetComponent<ParentComponent>().Children;
			for (Entity e : children)
				DestroyEntity(e);
		}

		if (entity.HasComponent<ChildComponent>())
		{
			Entity parent = entity.GetComponent<ChildComponent>().Parent;
			auto& parentChildren = parent.GetComponent<ParentComponent>().Children;

			DeleteFromChildren(parentChildren, parent, entity);
		}

		m_EntityMap.erase(entity.GetID());
		m_Registry.destroy(entity);
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		String name = entity.GetName();
		Entity newEntity;

		if (entity.HasComponent<ChildComponent>())
		{
			Entity entityParent = entity.GetComponent<ChildComponent>().Parent;
			newEntity = CreateEntity(name, UUID(), entityParent);
		}
		else
		{
			newEntity = CreateEntity(name, UUID());
		}

		CopyComponentIfExists(AllComponents{}, newEntity, entity);

		if (entity.HasComponent<ParentComponent>())
		{
			// Copy because next calls of DuplicateEntity in for-loop can reallocate this array
			std::vector<Entity> children = entity.GetComponent<ParentComponent>().Children;

			// Clear new children because, ParentComponent copied in CopyComponentIfExists
			std::vector<Entity>& newChildren = newEntity.GetComponent<ParentComponent>().Children;
			newChildren.clear();

			for (Entity child : children)
			{
				Entity newChild = DuplicateEntity(child);
				MakeRelationship(newEntity, newChild);
			}
		}

		return newEntity;
	}

	void Scene::MakeRelationship(Entity parent, Entity child)
	{
		if (child.HasComponent<ChildComponent>())
		{
			Entity& oldParent = child.GetComponent<ChildComponent>().Parent;
			auto& oldParentChildren = oldParent.GetComponent<ParentComponent>().Children;

			DeleteFromChildren(oldParentChildren, oldParent, child);
			oldParent = parent;
		}
		else
		{
			child.AddComponent<ChildComponent>().Parent = parent;
		}

		if (!parent.HasComponent<ParentComponent>())
			parent.AddComponent<ParentComponent>();

		auto& children = parent.GetComponent<ParentComponent>().Children;
		children.push_back(child);
	}

	void Scene::MakeOrphan(Entity child)
	{
		if (child.HasComponent<ChildComponent>())
		{
			Entity parent = child.GetComponent<ChildComponent>().Parent;
			auto& parentChildren = parent.GetComponent<ParentComponent>().Children;

			DeleteFromChildren(parentChildren, parent, child);
			child.RemoveComponent<ChildComponent>();
		}
	}

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		ATN_CORE_VERIFY(m_EntityMap.find(uuid) != m_EntityMap.end());

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
				PrivateScriptEngine::OnUpdateEntity(entity, frameTime);
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
		SceneCamera* mainCamera = nullptr;
		TransformComponent cameraTransform;
		{
			auto cameras = m_Registry.view<CameraComponent>();
			for (auto entity : cameras)
			{
				auto& camera = cameras.get<CameraComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = GetWorldTransform(entity);
					break;
				}
			}
		}

		// Render 
		{
			if (mainCamera)
			{
				RenderRuntimeScene(*mainCamera, cameraTransform.AsMatrix());
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
			PrivateScriptEngine::OnRuntimeStart(this);

			// Instantiate all script entities
			auto view = m_Registry.view<ScriptComponent>();
			for (auto id: view)
			{
				Entity entity = { id, this };
				PrivateScriptEngine::InstantiateEntity(entity);
			}

			for (auto id: view)
			{
				Entity entity = { id, this };
				PrivateScriptEngine::OnCreateEntity(entity);
			}
		}
	}

	void Scene::OnSimulationStart()
	{
		OnPhysics2DStart();
	}

	void Scene::LoadAllScripts()
	{
		auto view = m_Registry.view<ScriptComponent>();
		for (auto id : view)
		{
			Entity entity = { id, this };
			auto& scriptComponent = view.get<ScriptComponent>(entity);
			PrivateScriptEngine::LoadScript(scriptComponent.Name, entity);
		}
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
		m_Registry.view<Rigidbody2DComponent>().each([this](auto entityID, auto& rb2d)
			{
				Entity entity = Entity(entityID, this);
				TransformComponent transform = entity.GetWorldTransform();

				b2BodyDef bodyDef;
				bodyDef.type = AthenaRigidBody2DTypeToBox2D(rb2d.Type);
				bodyDef.position.Set(transform.Translation.x, transform.Translation.y);
				bodyDef.angle = transform.Rotation.AsEulerAngles().z;

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

		auto rigidBodies2D = GetAllEntitiesWith<Rigidbody2DComponent>();
		for (auto entityID : rigidBodies2D)
		{
			Entity entity = { entityID, this };
			TransformComponent oldWorldTransform = entity.GetWorldTransform();
			TransformComponent newWorldTransform = oldWorldTransform;

			const auto& rb2D = rigidBodies2D.get<Rigidbody2DComponent>(entity);

			b2Body* body = reinterpret_cast<b2Body*>(rb2D.RuntimeBody);
			const auto& position = body->GetPosition();
			newWorldTransform.Translation.x = position.x;
			newWorldTransform.Translation.y = position.y;

			Vector3 eulerAngles = oldWorldTransform.Rotation.AsEulerAngles();
			eulerAngles.z = body->GetAngle();
			newWorldTransform.Rotation = Quaternion(eulerAngles);

			entity.GetComponent<TransformComponent>().UpdateFromWorldTransforms(newWorldTransform, oldWorldTransform);
		}
	}

	void Scene::RenderEditorScene(const EditorCamera& camera)
	{
		Matrix4 viewMatrix = camera.GetViewMatrix();
		Matrix4 projectionMatrix = camera.GetProjectionMatrix();
		float near = camera.GetNearClip();
		float far = camera.GetFarClip();

		RenderScene(viewMatrix, projectionMatrix, near, far);

		// Render Entity IDs
		RenderPass renderPass;
		renderPass.TargetFramebuffer = SceneRenderer::GetEntityIDFramebuffer();
		renderPass.ClearBit = CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT;
		renderPass.Name = "EntityIDs";

		Renderer::BeginRenderPass(renderPass);
		{
			SceneRenderer::GetEntityIDFramebuffer()->ClearAttachment(0, -1);

			SceneRenderer::FlushEntityIDs();

			SceneRenderer2D::EntityIDEnable(true);
			SceneRenderer2D::BeginScene(viewMatrix, projectionMatrix);

			auto quads = GetAllEntitiesWith<SpriteComponent>();
			for (auto entity : quads)
			{
				auto transform = GetWorldTransform(entity);
				const auto& sprite = quads.get<SpriteComponent>(entity);

				SceneRenderer2D::DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Color, sprite.TilingFactor, (int32)entity);
			}

			auto circles = GetAllEntitiesWith<CircleComponent>();
			for (auto entity : circles)
			{
				auto transform = GetWorldTransform(entity);
				const auto& circle = circles.get<CircleComponent>(entity);

				SceneRenderer2D::DrawCircle(transform.AsMatrix(), circle.Color, circle.Thickness, circle.Fade, (int32)entity);
			}

			SceneRenderer2D::EndScene();
			SceneRenderer2D::EntityIDEnable(false);

		}
		Renderer::EndRenderPass();
	}

	void Scene::RenderRuntimeScene(const SceneCamera& camera, const Matrix4& transform)
	{
		Matrix4 viewMatrix = Math::AffineInverse(transform);
		Matrix4 projectionMatrix = camera.GetProjectionMatrix();
		float near = camera.GetNearClip();
		float far = camera.GetFarClip();

		RenderScene(viewMatrix, projectionMatrix, near, far);
	}

	void Scene::RenderScene(const Matrix4& view, const Matrix4& proj, float near, float far)
	{
		SceneRenderer::BeginScene({ view, proj, near, far }, m_Environment);

		auto staticMeshes = GetAllEntitiesWith<StaticMeshComponent>();
		for (auto entity : staticMeshes)
		{
			auto transform = GetWorldTransform(entity);
			const auto& meshComponent = staticMeshes.get<StaticMeshComponent>(entity);

			if (meshComponent.Visible)
			{
				const auto& subMeshes = meshComponent.Mesh->GetAllSubMeshes();
				for (uint32 i = 0; i < subMeshes.size(); ++i)
				{
					Ref<Material> material = MaterialManager::Get(subMeshes[i].MaterialName);
					Ref<Animator> animator = meshComponent.Mesh->GetAnimator();

					if(animator != nullptr && animator->IsPlaying())
						SceneRenderer::Submit(subMeshes[i].VertexBuffer, material, animator, transform.AsMatrix(), (int32)entity);
					else
						SceneRenderer::Submit(subMeshes[i].VertexBuffer, material, nullptr, transform.AsMatrix(), (int32)entity);
				}
			}
		}

		auto dirLights = GetAllEntitiesWith<DirectionalLightComponent>();
		for (auto entity : dirLights)
		{
			auto transform = GetWorldTransform(entity);
			const auto& light = dirLights.get<DirectionalLightComponent>(entity);

			Vector3 direction = transform.Rotation * Vector3::Forward();
			DirectionalLight dirLight = { light.Color, direction, light.Intensity };
			SceneRenderer::SubmitLight(dirLight);
		}

		auto pointLights = GetAllEntitiesWith<PointLightComponent>();
		for (auto entity : pointLights)
		{
			auto transform = GetWorldTransform(entity);
			const auto& light = pointLights.get<PointLightComponent>(entity);

			PointLight pointLight = { light.Color, transform.Translation, light.Intensity, light.Radius, light.FallOff };
			SceneRenderer::SubmitLight(pointLight);
		}

		SceneRenderer::EndScene();


		SceneRenderer2D::BeginScene(view, proj);

		auto quads = GetAllEntitiesWith<SpriteComponent>();
		for (auto entity : quads)
		{
			auto transform = GetWorldTransform(entity);
			const auto& sprite = quads.get<SpriteComponent>(entity);

			SceneRenderer2D::DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Color, sprite.TilingFactor);
		}

		auto circles = GetAllEntitiesWith<CircleComponent>();
		for (auto entity : circles)
		{
			auto transform = GetWorldTransform(entity);
			const auto& circle = circles.get<CircleComponent>(entity);

			SceneRenderer2D::DrawCircle(transform.AsMatrix(), circle.Color, circle.Thickness, circle.Fade);
		}

		SceneRenderer2D::EndScene();
	}

	TransformComponent Scene::GetWorldTransform(entt::entity enttentity)
	{
		Entity entity = { enttentity, this };
		return entity.GetWorldTransform();
	}
}
