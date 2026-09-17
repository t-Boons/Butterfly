#include "Scene/Scene.hpp"
#include "Scene/Registry/PendingDestroyComponent.hpp"
#include "Scene/Registry/IDComponent.hpp"
#include "Scene/Registry/NameComponent.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Serialization/ENTT/ComponentRegistry.hpp"

#include "Core/Application.hpp"
#include "Asset/AssetManager.hpp"

namespace Butterfly
{
	Scene::Scene()
	{
		m_rootEntity = CreateEntity("Root");
	}

	YAML::Node Scene::Serialize()
	{
		YAML::Node root;
		root["Scene"] = "Untitled Scene";
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

		YAML::Emitter emit;
		emit << root;
		
		std::string text = emit.c_str();
		AssetMetadata meta;
		Application::Get().GetAssetManager().GetAssetRegistry().NewFile("NewScene.bfscene", text, meta);
		return root;
	}

	void Scene::Deserialize(const std::string& text)
	{
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
	}

	void Scene::Tick()
	{
		DestroyPendingEntities();
	}

	void Scene::DestroyChildren(entt::entity entity)
	{
		TransformComponent& tr = m_entityRegistry.get<TransformComponent>(entity);

		const auto children = tr.GetChildren();

		for (auto child : children)
		{
			DestroyChildren(child.GetHandle());
		}

		tr.DetachParent();
		m_entityRegistry.destroy(entity);
	}
		
	void Scene::DestroyPendingEntities()
	{
		auto view = m_entityRegistry.view<PendingDestroyComponent>();

		for (auto& entity : view)
		{
			DestroyChildren(entity);
		}
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		// Add the entity to the registry and add the required components.
		Entity entity(&m_entityRegistry, m_entityRegistry.create());

		TransformComponent& tr = entity.AddComponent<TransformComponent>();
		tr.m_thisEntity = entity;

		// Attach it to the scene root.
		if (m_rootEntity)
		{
			TransformComponent& rootTransform = m_rootEntity.GetComponent<TransformComponent>();
			rootTransform.Attach(tr);
		}

		entity.AddComponent<IDComponent>().EntityUUID = UUID::Generate();

		// Make sure there are no duplicate names in the scene, if there are, append a number to the end of the name.
		NameComponent nameComponent;
		nameComponent.Tag = "Untagged";
		nameComponent.Name = name;
		
		bool duplicateNameFound = true;
		while (duplicateNameFound)
		{
			duplicateNameFound = false;
			for (const auto& [entityName, existingNameComponent] : m_entityRegistry.view<NameComponent>().each())
			{
				if (nameComponent.Name == existingNameComponent.Name)
				{
					nameComponent.Name = Utils::IterateDuplicateName(existingNameComponent.Name);
					duplicateNameFound = true;
					break;
				}
			}
		}
		entity.AddComponent<NameComponent>(nameComponent);


		return entity;
	}
}