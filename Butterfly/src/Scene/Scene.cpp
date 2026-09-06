#include "Scene/Scene.hpp"
#include "Scene/Registry/PendingDestroy.hpp"
#include "Scene/Registry/EntityID.hpp"

namespace Butterfly
{
	void Scene::Init()
	{

	}

	void Scene::Tick()
	{
		DestroyPendingEntities();
	}
		
	void Scene::DestroyPendingEntities()
	{
		auto view = m_entityRegistry.view<PendingDestroy>();

		for (auto& entity : view)
		{
			m_entityRegistry.destroy(entity);
		}
	}

	Entity Scene::CreateEntity()
	{
		Entity entity(&m_entityRegistry);
		entity.AddComponent<EntityID>().Value = UUID::Generate();
		return entity;
	}
}