#pragma once
#include "Core/Common.hpp"
#include "Scene/Entity.hpp"
#include "entt/entt.hpp"

namespace Butterfly
{
	class Scene
	{
		Scene(const Scene& other)
		{
			for (auto& entity : m_entities)
			{
				entity.m_registry = other.m_registry;
			}
		}

		std::string m_name = "New Scene";
		entt::registry m_registry;
		std::vector<Entity> m_entities;
		Entity m_rootEntity;
	};

	class SceneManager : public NonCopyable
	{
	public:
		SceneManager();

		void Tick();

		YAML::Node Serialize();
		void Deserialize(const std::string& text, const std::string& name);

		void DestroyPendingEntities();
		Entity CreateEntity(const std::string& name = "New GameObject");
		entt::registry& GetEntityRegistry() { return m_entityRegistry; }
		const entt::registry& GetEntityRegistry() const { return m_entityRegistry; }
		const Entity& GetRootEntity() const { return m_rootEntity; }

	private:
		void DestroyChildren(entt::entity entity);

		std::string m_name = "New Scene";
		entt::registry m_entityRegistry;
		Entity m_rootEntity;
	};
}