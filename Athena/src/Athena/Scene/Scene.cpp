#include "atnpch.h"

#include "Scene.h"
#include "Entity.h"

#include "Components.h"
#include "Athena/Renderer/Renderer2D.h"


namespace Athena
{
	Scene::Scene()
	{

	}

	Scene::~Scene()
	{

	}

	Entity Scene::CreateEntity(const String& name)
	{
		Entity entity(m_Registry.create(), this);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<TagComponent>(name);

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void Scene::OnUpdate(Time frameTime)
	{
		//Run native scripts
		m_Registry.view<NativeScriptComponent>().each([=] (auto entity, auto& nsc) 
		{
			if (!nsc.Script)
			{
				nsc.Script = nsc.InstantiateScript();
				nsc.Script->m_Entity = Entity(entity, this);

				nsc.Script->Init();
			}

			nsc.Script->OnUpdate(frameTime);
		});

		// Choose camera
		Camera* mainCamera = nullptr;
		Matrix4 cameraTransform;
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
		// Render 2D
		if (mainCamera)
		{
			Renderer2D::BeginScene(*mainCamera, cameraTransform);

			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteComponent>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponent, SpriteComponent>(entity);

				if (sprite.Texture.GetNativeTexture() != nullptr)
					Renderer2D::DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Color, sprite.TilingFactor);
				else
					Renderer2D::DrawQuad(transform.AsMatrix(), sprite.Color);
			}

			Renderer2D::EndScene();
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
}
