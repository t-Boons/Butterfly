#include "Scene/Scene.hpp"
#include "Scene/Registry/PendingDestroyComponent.hpp"
#include "Scene/Registry/IDComponent.hpp"
#include "Scene/Registry/NameComponent.hpp"
#include "Serialization/ENTT/ComponentRegistry.hpp"

#include "Core/Application.hpp"
#include "Asset/AssetManager.hpp"

namespace Butterfly
{
	Scene::Scene()
	{

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
		
	void Scene::DestroyPendingEntities()
	{
		auto view = m_entityRegistry.view<PendingDestroyComponent>();

		for (auto& entity : view)
		{
			m_entityRegistry.destroy(entity);
		}
	}

	Entity Scene::CreateEntity()
	{
		Entity entity(m_entityRegistry.create());
		entity.AddComponent<IDComponent>().EntityUUID = UUID::Generate();
		
		NameComponent name;
		name.Tag = "Untagged";
		name.Name = "New GameObject";
		entity.AddComponent<NameComponent>(name);

		return entity;
	}
}