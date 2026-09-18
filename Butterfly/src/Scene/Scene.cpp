#include "Scene/Scene.hpp"
#include "Scene/Registry/PendingDestroyComponent.hpp"
#include "Scene/Registry/IDComponent.hpp"
#include "Scene/Registry/NameComponent.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Serialization/ENTT/ComponentRegistry.hpp"
#include "Core/Window.hpp"

#include "Core/Application.hpp"
#include "Asset/AssetManager.hpp"

namespace Butterfly
{
	SceneManager::SceneManager()
	{
		BF_PROFILE_EVENT()

		m_rootEntity = CreateEntity("Root");
		Application::Get().GetWindow().SetWindowTitle("Butterfly Editor - " + m_name);
	}

	YAML::Node SceneManager::Serialize()
	{
		BF_PROFILE_EVENT()

		YAML::Node root;
		root["Scene"] = m_name;
		YAML::Node entitiesNode;
		auto view = m_entityRegistry.view<IDComponent>();
		for (auto& entity : view)
		{
			YAML::Node entityNode;

			for (auto& serializer : ComponentRegistry::GetComponentSerializers())
			{
				if (serializer.Has(m_entityRegistry, entity))
				{
					YAML::Node serializerNode;
					serializer.Serialize(serializerNode, m_entityRegistry, entity);
					entityNode["Entity"].push_back(serializerNode);
				}
			}

			entitiesNode.push_back(entityNode);
		}
		root["Scene"] = entitiesNode;

		return root;
	}

	void SceneManager::Deserialize(const std::string& text, const std::string& name)
	{
		BF_PROFILE_EVENT()

		m_name = name;
		m_entityRegistry.clear();

		YAML::Node node = YAML::Load(text);
		YAML::Node scene = node["Scene"];

		for (const auto& entityNode : scene)
		{
			entt::entity entity = m_entityRegistry.create();

			for (const auto& componentNode : entityNode["Entity"])
			{
				for (const auto& serializer : ComponentRegistry::GetComponentSerializers())
				{
					if (!componentNode[serializer.Name])
					{
						continue;
					}

					serializer.Deserialize(componentNode, m_entityRegistry, entity);
				}
			}
		}

		for (const auto& [entity, transform] : m_entityRegistry.view<TransformComponent>().each())
		{
			transform.ValidateAfterDeserialization(*this);
		}

		auto firstEntity = *m_entityRegistry.view<TransformComponent>().begin();
		// Get the root from any of the other existing transformcomponents since they are all parented to the root.
		m_rootEntity = m_entityRegistry.get<TransformComponent>(firstEntity).GetRoot();
		Application::Get().GetWindow().SetWindowTitle("Butterfly Editor - " + m_name);
	}

	void SceneManager::Tick()
	{
		BF_PROFILE_EVENT()

		DestroyPendingEntities();
	}

	void SceneManager::DestroyChildren(entt::entity entity)
	{
		BF_PROFILE_EVENT()

		TransformComponent& tr = m_entityRegistry.get<TransformComponent>(entity);

		const uint32_t numChildren = tr.NumChildren();

		for (uint32_t i = 0; i < numChildren; ++i)
		{
			DestroyChildren(tr.GetChild(i).GetHandle());
		}

		tr.DetachParent();
		m_entityRegistry.destroy(entity);
	}
		
	void SceneManager::DestroyPendingEntities()
	{
		BF_PROFILE_EVENT()

		auto view = m_entityRegistry.view<PendingDestroyComponent>();

		bool rootEntityPendingDestroy = false;
		for (auto& entity : view)
		{
			if (entity == m_rootEntity.GetHandle())
			{
				rootEntityPendingDestroy = true;
				BF_CORE_LOG_ERROR("SceneManager::DestroyPendingEntities: Cannot destroy root entity");
				continue;
			}

			DestroyChildren(entity);
		}

		if (rootEntityPendingDestroy)
		{
			m_rootEntity.RemoveComponent<PendingDestroyComponent>();
		}
	}

	Entity SceneManager::CreateEntity(const std::string& name)
	{
		BF_PROFILE_EVENT()

		// Add the entity to the registry and add the required components.
		Entity entity(&m_entityRegistry, m_entityRegistry.create());

		entity.AddComponent<IDComponent>().EntityUUID = UUID::Generate();

		TransformComponent& tr = entity.AddComponent<TransformComponent>();

		// Attach it to the scene root.
		if (m_rootEntity)
		{
			TransformComponent& rootTransform = m_rootEntity.GetComponent<TransformComponent>();
			rootTransform.Attach(tr);
		}


		// Make sure there are no duplicate names in the scene, if there are, append a number to the end of the name.
		
		std::string newEntityName = name;

		bool duplicateNameFound = true;
		while (duplicateNameFound)
		{
			duplicateNameFound = false;
			for (const auto& [entityName, existingNameComponent] : m_entityRegistry.view<NameComponent>().each())
			{
				if (newEntityName == existingNameComponent.Name)
				{
					newEntityName = Utils::IterateDuplicateName(existingNameComponent.Name);
					duplicateNameFound = true;
					break;
				}
			}
		}

		NameComponent& nameComponent = entity.AddComponent<NameComponent>();
		nameComponent.Tag = "Untagged";
		nameComponent.Name = newEntityName;

		return entity;
	}
}