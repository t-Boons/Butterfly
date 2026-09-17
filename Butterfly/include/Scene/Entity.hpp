#pragma once
#include "Core/Common.hpp"
#include "Scene/Registry/PendingDestroyComponent.hpp"

namespace Butterfly
{
	using EntityHandle = entt::entity;

	class Entity
	{
	public:
		Entity();
		Entity(entt::registry* registry, EntityHandle handle);

		Entity(const Entity& other)
			: m_registry(other.m_registry), m_handle(other.m_handle)
		{
		}

		Entity& operator =(const Entity& other)
		{
			m_registry = other.m_registry;
			m_handle = other.m_handle;
			return *this;
		}

		explicit operator bool() const
		{
			return Valid();
		}

		bool operator ==(const Entity& other) const
		{
			return m_handle == other.m_handle && m_registry == other.m_registry;
		}

		bool operator !=(const Entity& other) const
		{
			return !(*this == other);
		}

		bool Valid() const
		{
			return m_handle != entt::null && m_registry && m_registry->valid(m_handle);
		}

		EntityHandle GetHandle() const
		{
			BF_CORE_ASSERT(Valid(), "Entity::GetHandle: Entity is not valid");
			return m_handle;
		}

		template<typename T>
		T& AddComponent()
		{
			BF_CORE_ASSERT(m_registry, "Entity::AddComponent: Entity has no registry");
			BF_CORE_ASSERT(m_handle != entt::null, "Entity::AddComponent: Entity handle is null");
			BF_CORE_ASSERT(m_registry->valid(m_handle), "Entity::AddComponent: Entity is no longer valid");
			BF_CORE_ASSERT(!m_registry->all_of<T>(m_handle), "Entity::AddComponent: Entity already has this component");

			return m_registry->emplace<T>(m_handle, *this);
		}

		template<typename T>
		T& GetComponent() const
		{
			BF_CORE_ASSERT(HasComponent<T>(), "Entity::GetComponent: Entity does not have this component");

			return m_registry->get<T>(m_handle);
		}

		template<typename T>
		bool HasComponent() const
		{
			BF_CORE_ASSERT(m_registry, "Entity::HasComponent: Entity has no registry");
			BF_CORE_ASSERT(m_handle != entt::null, "Entity::HasComponent: Entity handle is null");
			BF_CORE_ASSERT(m_registry->valid(m_handle), "Entity::HasComponent: Entity is no longer valid");

			return m_registry->all_of<T>(m_handle);
		}

		template<typename T>
		T* TryGetComponent() const
		{
			BF_CORE_ASSERT(m_registry, "Entity::TryGetComponent: Entity has no registry");
			BF_CORE_ASSERT(m_handle != entt::null, "Entity::TryGetComponent: Entity handle is null");
			BF_CORE_ASSERT(m_registry->valid(m_handle), "Entity::TryGetComponent: Entity is no longer valid");

			if (!m_registry->all_of<T>(m_handle))
			{
				return nullptr;
			}

			return &m_registry->get<T>(m_handle);
		}

		void Destroy()
		{
			BF_CORE_ASSERT(m_registry, "Entity::Destroy: Entity has no registry");
			BF_CORE_ASSERT(m_handle != entt::null, "Entity::Destroy: Entity handle is null");
			BF_CORE_ASSERT(m_registry->valid(m_handle), "Entity::Destroy: Entity is no longer valid");
			BF_CORE_ASSERT(!m_registry->all_of<PendingDestroyComponent>(m_handle), "Entity::Destroy: Entity is already pending destruction");

			m_registry->emplace<PendingDestroyComponent>(m_handle);
		}

	private:
		entt::registry* m_registry = nullptr;
		EntityHandle m_handle = entt::null;
	};
}