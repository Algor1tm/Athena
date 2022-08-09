#include "atnpch.h"
#include "Scene.h"


namespace Athena
{
	Scene::Scene()
	{
		entt::entity entity = m_Registry.create();
	}

	Scene::~Scene() 
	{

	}
}
