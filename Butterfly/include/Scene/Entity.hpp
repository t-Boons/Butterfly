#pragma once
#include "Core/Common.hpp"
#include "Scene/Registry/PendingDestroyComponent.hpp"

namespace Butterfly
{
	using EntityHandle = entt::entity;

	class Entity : public NonMoveable
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
			return !(m_handle == other.m_handle && m_registry == other.m_registry);
		}


		bool Valid() const
		{
			return m_handle != entt::null && m_registry && m_registry->valid(m_handle);
		}
		
		EntityHandle GetHandle() const { return m_handle; }

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			BF_CORE_ASSERT(m_handle != entt::null, "Entity is not valid!");
			return m_registry->emplace<T>(m_handle,std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent() const
		{
			BF_CORE_ASSERT(m_handle != entt::null, "Entity is not valid!");
			return m_registry->get<T>(m_handle);
		}

		template<typename T>
		bool HasComponent() const
		{
			BF_CORE_ASSERT(m_handle != entt::null, "Entity is not valid!");
			return m_registry->all_of<T>(m_handle);
		}

		template<typename T>
		T* TryGetComponent() const
		{
			BF_CORE_ASSERT(m_handle != entt::null, "Entity is not valid!");
			if (HasComponent<T>())
			{
				return &m_registry->get<T>(m_handle);
			}
			return nullptr;
		}

		void Destroy()
		{
			BF_CORE_ASSERT(m_handle != entt::null, "Entity is not valid!");
			m_registry->emplace<PendingDestroyComponent>(m_handle);
		}
		
	private:
		entt::registry* m_registry;
		EntityHandle m_handle;
	};
}