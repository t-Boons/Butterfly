#pragma once
#include "Core/Common.hpp"
#include "Scene/Registry/PendingDestroyComponent.hpp"

namespace Butterfly
{
	using EntityHandle = entt::entity;

	class Entity
	{
	public:
		Entity()
			: m_registry(nullptr)
		{

		}

		Entity(entt::registry* registry)
			: m_registry(registry)
		{
			m_handle = m_registry->create();
		}

		explicit operator bool() const
		{
			return m_registry && m_registry->valid(m_handle);
		}

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			return m_registry->emplace<T>(m_handle,std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			return m_registry->get<T>(m_handle);
		}

		template<typename T>
		bool HasComponent() const
		{
			return m_registry->all_of<T>(m_handle);
		}

		void Destroy()
		{
			m_registry->emplace<PendingDestroyComponent>(m_handle);
		}

	private:
		entt::registry* m_registry;
		EntityHandle m_handle;
	};
}