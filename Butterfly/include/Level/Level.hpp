#pragma once
#include "Core/Common.hpp"
#include "Level/Registry/PendingDestroyComponent.hpp"

namespace Butterfly
{
	class Level : public NonCopyableNonMoveable
	{
	public:
		static std::string Serialize(const Level& level);
		static RefPtr<Level> Deserialize(const std::string& text);
		static RefPtr<Level> DeserializeFromFilePath(const std::filesystem::path& filePath);

		void CloneTo(Level& destination) const;

		template<typename T>
		T& AddComponent(entt::entity entity)
		{
			BF_CORE_ASSERT(entity != entt::null, "Entity::AddComponent: Entity handle is null");
			BF_CORE_ASSERT(m_registry.valid(entity), "Entity::AddComponent: Entity is no longer valid");
			BF_CORE_ASSERT(!m_registry.all_of<T>(entity), "Entity::AddComponent: Entity already has this component");

			return m_registry.emplace<T>(entity, entity);
		}

		template<typename T>
		T& GetComponent(entt::entity entity)
		{
			BF_CORE_ASSERT(HasComponent<T>(entity), "Entity::GetComponent: Entity does not have this component");

			return m_registry.get<T>(entity);
		}

		template<typename T>
		bool HasComponent(entt::entity entity) const
		{
			BF_CORE_ASSERT(entity != entt::null, "Entity::HasComponent: Entity handle is null");
			BF_CORE_ASSERT(m_registry.valid(entity), "Entity::HasComponent: Entity is no longer valid");

			return m_registry.all_of<T>(entity);
		}

		template<typename T>
		T* TryGetComponent(entt::entity entity)
		{
			BF_CORE_ASSERT(entity != entt::null, "Entity::TryGetComponent: Entity handle is null");
			BF_CORE_ASSERT(m_registry.valid(entity), "Entity::TryGetComponent: Entity is no longer valid");

			if (!m_registry.all_of<T>(entity))
			{
				return nullptr;
			}

			return &m_registry.get<T>(entity);
		}

		template<typename T>
		void RemoveComponent(entt::entity entity)
		{
			BF_CORE_ASSERT(HasComponent<T>(entity), "Entity::RemoveComponent: Entity does not have this component");
			m_registry.remove<T>(entity);
		}

		void DestroyEntity(entt::entity entity)
		{
			BF_CORE_ASSERT(entity != entt::null, "Entity::Destroy: Entity handle is null");
			BF_CORE_ASSERT(m_registry.valid(entity), "Entity::Destroy: Entity is no longer valid");
			BF_CORE_ASSERT(!m_registry.all_of<PendingDestroyComponent>(entity), "Entity::Destroy: Entity is already pending destruction");

			m_registry.emplace<PendingDestroyComponent>(entity);
		}

		const entt::registry& GetRegistry() const { return m_registry; }
		const std::string& GetName() const { return m_name; }

		std::string m_name;
		entt::registry m_registry;
		entt::entity m_rootEntity;
	};

	class LevelManager : public NonCopyable
	{
	public:
		LevelManager();

		void Tick();


		void SaveCurrentLevel();
		void SetActiveLevel(const RefPtr<Level>& level);
		const Level& GetActiveLevel() const { return *m_activeLevel; }
		Level& GetActiveLevel() { return *m_activeLevel; }

		void DestroyPendingEntities();
		entt::entity CreateEntity(const std::string& name = "New GameObject");
		entt::registry& GetEntityRegistry() { return m_activeLevel->m_registry; }
		const entt::registry& GetEntityRegistry() const { return m_activeLevel->m_registry; }
		const entt::entity& GetRootEntity() const { return m_activeLevel->m_rootEntity; }

	private:
		void DestroyChildren(entt::entity entity);

		RefPtr<Level> m_activeLevel;
	};
}
