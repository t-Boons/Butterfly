#include "Scene/Scene.hpp"
#include "Scene/Registry/PendingDestroy.hpp"
#include "Scene/Registry/IDComponent.hpp"
#include "Scene/Registry/NameComponent.hpp"

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
		entity.AddComponent<IDComponent>().Value = UUID::Generate();
		
		NameComponent name;
		name.Tag = "Untagged";
		name.Name = "New GameObject";
		entity.AddComponent<NameComponent>(name);

		return entity;
	}
}