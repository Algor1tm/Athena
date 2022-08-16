#include "atnpch.h"

#include "Entity.h"
#include "Scene.h"

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

	Entity Scene::CreateEntity(std::string_view name)
	{
		Entity entity(m_Registry.create(), this);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<TagComponent>(name);

		return entity;
	}

	void Scene::OnUpdate(Time frameTime)
	{
		auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
		for (auto entity : group)
		{
			auto& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

			Renderer2D::DrawQuad(transform, sprite.Color);
		}
	}
}
