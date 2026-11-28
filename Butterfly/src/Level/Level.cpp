#include "Level/Level.hpp"
#include "Level/Registry/PendingDestroyComponent.hpp"
#include "Level/Registry/IDComponent.hpp"
#include "Level/Registry/NameComponent.hpp"
#include "Level/Registry/TransformComponent.hpp"
#include "Serialization/ENTT/ComponentRegistry.hpp"
#include "Core/Window.hpp"
#include "Renderer/Renderer.hpp"

#include "Core/Application.hpp"
#include "Asset/AssetManager.hpp"

#include "Level/Entity.hpp"

#include "Core/FileSystem.hpp"

namespace Butterfly
{
	std::string Level::Serialize(const Level& level)
	{
		BF_PROFILE_EVENT()

		YAML::Node root;
		YAML::Node entitiesNode;

		const auto view = level.m_registry.view<IDComponent>();

		for (auto entity : view)
		{
			YAML::Node entityNode;
			YAML::Node entityDataNode = entityNode["Entity"];
			ComponentRegistry::SerializeComponents(entityDataNode, level.m_registry, entity);

			entitiesNode.push_back(entityNode);
		}

		root["LevelName"] = level.m_name;
		root["Level"] = entitiesNode;

		YAML::Emitter out;
		out << root;
		return out.c_str();
	}

	RefPtr<Level> Level::DeserializeFromFilePath(const std::filesystem::path& filePath)
	{
		BF_PROFILE_EVENT()

		return Deserialize(FileSystem::ReadText(filePath));
	}

	RefPtr<Level> Level::Deserialize(const std::string& text)
	{
		BF_PROFILE_EVENT()

		RefPtr<Level> out = MakeRef<Level>();
		YAML::Node node = YAML::Load(text);
		YAML::Node level = node["Level"];
		out->m_name = node["LevelName"].as<std::string>();
		out->m_registry.clear();


		for (const auto& entityNode : level)
		{
			entt::entity entity = out->m_registry.create();

			ComponentRegistry::DeserializeComponents(entityNode["Entity"], out->m_registry, entity);
		}

		for (const auto& [entity, transform] : out->m_registry.view<TransformComponent>().each())
		{
			transform.ValidateAfterDeserialization(*out);
		}

		// Find the root entity (the one without a parent) and set it as the level's root entity.
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

	void Level::CloneTo(Level& destination) const
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



	LevelManager::LevelManager()
	{
		BF_PROFILE_EVENT()

		m_activeLevel = MakeRef<Level>();
		m_activeLevel->m_rootEntity = CreateEntity("Root");
		m_activeLevel->m_name = "Untitled Level";

		Application::Get().GetWindow().SetWindowTitle("Butterfly Editor - " + m_activeLevel->GetName());
	}

	void LevelManager::SaveCurrentLevel()
	{
		BF_PROFILE_EVENT()

		AssetFileMetadata meta;
		Application::Get().GetAssetManager().GetAssetRegistry().NewFile(m_activeLevel->GetName(), ".bflevel", Level::Serialize(*m_activeLevel), meta);
	}

	void LevelManager::SetActiveLevel(const RefPtr<Level>& level)
	{
		BF_PROFILE_EVENT()

		// Unload previous level.
		m_activeLevel.reset();

		m_activeLevel = level;
		Application::Get().GetWindow().SetWindowTitle("Butterfly Editor - " + m_activeLevel->GetName());
	}

	void LevelManager::Tick()
	{
		BF_PROFILE_EVENT()

		DestroyPendingEntities();
	}

	void LevelManager::DestroyChildren(entt::entity entity)
	{
		BF_PROFILE_EVENT()

		TransformComponent& tr = m_activeLevel->m_registry.get<TransformComponent>(entity);

		const uint32_t numChildren = tr.NumChildren();

		for (uint32_t i = 0; i < numChildren; ++i)
		{
			DestroyChildren(tr.GetChild(i).GetHandle());
		}

		tr.DetachParent();
		m_activeLevel->m_registry.destroy(entity);
	}
		
	void LevelManager::DestroyPendingEntities()
	{
		BF_PROFILE_EVENT()

		auto view = m_activeLevel->m_registry.view<PendingDestroyComponent>();

		bool rootEntityPendingDestroy = false;
		for (auto& entity : view)
		{
			if (entity == m_activeLevel->m_rootEntity)
			{
				rootEntityPendingDestroy = true;
				BF_CORE_LOG_ERROR("LevelManager::DestroyPendingEntities: Cannot destroy root entity");
				continue;
			}
			DestroyChildren(entity);
		}

		if (rootEntityPendingDestroy)
		{
			m_activeLevel->m_registry.remove<PendingDestroyComponent>(m_activeLevel->m_rootEntity);
		}
	}

	entt::entity LevelManager::CreateEntity(const std::string& name)
	{
		BF_PROFILE_EVENT()

		// Add the entity to the registry and add the required components.
		entt::entity entity = m_activeLevel->m_registry.create();

		IDComponent& id = m_activeLevel->AddComponent<IDComponent>(entity);
		id.EntityUUID = UUID::Generate();

		TransformComponent& tr = m_activeLevel->AddComponent<TransformComponent>(entity);

		// Attach it to the level root.
		if (m_activeLevel->m_rootEntity != entt::null)
		{
			TransformComponent& rootTransform = m_activeLevel->m_registry.get<TransformComponent>(m_activeLevel->m_rootEntity);
			rootTransform.Attach(tr);
		}


		// Make sure there are no duplicate names in the level, if there are, append a number to the end of the name.
		
		std::string newEntityName = name;

		bool duplicateNameFound = true;
		while (duplicateNameFound)
		{
			duplicateNameFound = false;
			for (const auto& [entityName, existingNameComponent] : m_activeLevel->m_registry.view<NameComponent>().each())
			{
				if (newEntityName == existingNameComponent.Name)
				{
					newEntityName = Utils::IterateDuplicateName(existingNameComponent.Name);
					duplicateNameFound = true;
					break;
				}
			}
		}

		NameComponent& nameComponent = m_activeLevel->AddComponent<NameComponent>(entity);
		nameComponent.Tag = "Untagged";
		nameComponent.Name = newEntityName;

		return entity;
	}
}
