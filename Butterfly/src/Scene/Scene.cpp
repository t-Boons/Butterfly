#include "Scene/Scene.hpp"
#include "Scene/Registry/PendingDestroyComponent.hpp"
#include "Scene/Registry/IDComponent.hpp"
#include "Scene/Registry/NameComponent.hpp"
#include "Scene/Registry/TransformComponent.hpp"
#include "Serialization/ENTT/ComponentRegistry.hpp"
#include "Core/Window.hpp"

#include "Core/Application.hpp"
#include "Asset/AssetManager.hpp"

#include "Scene/Entity.hpp"

#include "Core/FileSystem.hpp"

namespace Butterfly
{
	std::string Scene::Serialize(const Scene& scene)
	{
		BF_PROFILE_EVENT()

		YAML::Node root;
		YAML::Node entitiesNode;

		const auto view = scene.m_registry.view<IDComponent>();

		for (auto entity : view)
		{
			YAML::Node entityNode;
			YAML::Node entityDataNode = entityNode["Entity"];
			ComponentRegistry::SerializeComponents(entityDataNode, scene.m_registry, entity);

			entitiesNode.push_back(entityNode);
		}

		root["SceneName"] = scene.m_name;
		root["Scene"] = entitiesNode;

		YAML::Emitter out;
		out << root;
		return out.c_str();
	}

	RefPtr<Scene> Scene::Deserialize(const std::string& text)
	{
		BF_PROFILE_EVENT()

		RefPtr<Scene> out = MakeRef<Scene>();
		YAML::Node node = YAML::Load(text);
		YAML::Node scene = node["Scene"];
		out->m_name = node["SceneName"].as<std::string>();
		out->m_registry.clear();


		for (const auto& entityNode : scene)
		{
			entt::entity entity = out->m_registry.create();

			ComponentRegistry::DeserializeComponents(entityNode["Entity"], out->m_registry, entity);
		}

		for (const auto& [entity, transform] : out->m_registry.view<TransformComponent>().each())
		{
			transform.ValidateAfterDeserialization(*out);
		}

		// Find the root entity (the one without a parent) and set it as the scene's root entity.
		for (const auto& [entity, transform] : out->m_registry.view<TransformComponent>().each())
		{
			if (!transform.GetParent())
			{
				out->m_rootEntity = entity;
				break;
			}
		}
		return out;
	}

	void Scene::CloneTo(Scene& destination) const
	{
		const entt::registry& source = m_registry;
		entt::registry& target = destination.m_registry;

		target.clear();

		for (const auto& [src] : source.storage<entt::entity>()->each())
		{
			entt::entity dst = target.create(src);

			ComponentRegistry::RunOnAllComponents([&]<typename T>()
			{
				if (source.all_of<T>(src))
				{
					target.emplace_or_replace<T>(dst, source.get<T>(src));
				}
			});
		}

		destination.m_rootEntity = m_rootEntity;
	}



	SceneManager::SceneManager()
	{
		BF_PROFILE_EVENT()

		m_activeScene = MakeRef<Scene>();
		m_activeScene->m_rootEntity = CreateEntity("Root");
		m_activeScene->m_name = "Untitled Scene";

		Application::Get().GetWindow().SetWindowTitle("Butterfly Editor - " + m_activeScene->GetName());
	}

	void SceneManager::SaveCurrentScene()
	{
		BF_PROFILE_EVENT()

		AssetFileMetadata meta;
		Application::Get().GetAssetManager().GetAssetRegistry().NewFile(m_activeScene->GetName(), ".bfscene", Scene::Serialize(*m_activeScene), meta);
	}

	void SceneManager::LoadSceneFromFile(const std::filesystem::path& path)
	{
		BF_PROFILE_EVENT()

		m_activeScene.reset();
		m_activeScene = Scene::Deserialize(FileSystem::ReadText(path));
		Application::Get().GetWindow().SetWindowTitle("Butterfly Editor - " + m_activeScene->GetName());
	}

	void SceneManager::Tick()
	{
		BF_PROFILE_EVENT()

		DestroyPendingEntities();
	}

	void SceneManager::DestroyChildren(entt::entity entity)
	{
		BF_PROFILE_EVENT()

		TransformComponent& tr = m_activeScene->m_registry.get<TransformComponent>(entity);

		const uint32_t numChildren = tr.NumChildren();

		for (uint32_t i = 0; i < numChildren; ++i)
		{
			DestroyChildren(tr.GetChild(i).GetHandle());
		}

		tr.DetachParent();
		m_activeScene->m_registry.destroy(entity);
	}
		
	void SceneManager::DestroyPendingEntities()
	{
		BF_PROFILE_EVENT()

		auto view = m_activeScene->m_registry.view<PendingDestroyComponent>();

		bool rootEntityPendingDestroy = false;
		for (auto& entity : view)
		{
			if (entity == m_activeScene->m_rootEntity)
			{
				rootEntityPendingDestroy = true;
				BF_CORE_LOG_ERROR("SceneManager::DestroyPendingEntities: Cannot destroy root entity");
				continue;
			}

			DestroyChildren(entity);
		}

		if (rootEntityPendingDestroy)
		{
			m_activeScene->m_registry.remove<PendingDestroyComponent>(m_activeScene->m_rootEntity);
		}
	}

	entt::entity SceneManager::CreateEntity(const std::string& name)
	{
		BF_PROFILE_EVENT()

		// Add the entity to the registry and add the required components.
		entt::entity entity = m_activeScene->m_registry.create();

		IDComponent& id = m_activeScene->AddComponent<IDComponent>(entity);
		id.EntityUUID = UUID::Generate();

		TransformComponent& tr = m_activeScene->AddComponent<TransformComponent>(entity);

		// Attach it to the scene root.
		if (m_activeScene->m_rootEntity != entt::null)
		{
			TransformComponent& rootTransform = m_activeScene->m_registry.get<TransformComponent>(m_activeScene->m_rootEntity);
			rootTransform.Attach(tr);
		}


		// Make sure there are no duplicate names in the scene, if there are, append a number to the end of the name.
		
		std::string newEntityName = name;

		bool duplicateNameFound = true;
		while (duplicateNameFound)
		{
			duplicateNameFound = false;
			for (const auto& [entityName, existingNameComponent] : m_activeScene->m_registry.view<NameComponent>().each())
			{
				if (newEntityName == existingNameComponent.Name)
				{
					newEntityName = Utils::IterateDuplicateName(existingNameComponent.Name);
					duplicateNameFound = true;
					break;
				}
			}
		}

		NameComponent& nameComponent = m_activeScene->AddComponent<NameComponent>(entity);
		nameComponent.Tag = "Untagged";
		nameComponent.Name = newEntityName;

		return entity;
	}
}