#pragma once
#include "Core/Common.hpp"
#include "Scene/Registry/PendingDestroyComponent.hpp"

namespace Butterfly
{
	using EntityHandle = entt::entity;

	class Entity
	{
	public:
		Entity(EntityHandle handle = entt::null);

		explicit operator bool() const
		{
			return m_handle != entt::null && m_registry->valid(m_handle);
		}

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			BF_CORE_ASSERT(m_handle != entt::null, "Entity is not valid!");
			return m_registry->emplace<T>(m_handle,std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
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
		T* TryGetComponent()
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