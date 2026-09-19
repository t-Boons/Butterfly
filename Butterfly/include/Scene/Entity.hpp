#pragma once
#include "Core/Common.hpp"
#include "Core/Application.hpp"
#include "Scene/Scene.hpp"

namespace Butterfly
{
	class Entity
	{
	public:
		Entity();
		Entity(entt::entity handle);

		Entity(const Entity& other)
			: m_handle(other.m_handle)
		{
		}

		Entity& operator =(const Entity& other)
		{
			m_handle = other.m_handle;
			return *this;
		}

		explicit operator bool() const
		{
			return Valid();
		}

		bool operator ==(const Entity& other) const
		{
			return m_handle == other.m_handle;
		}

		bool operator !=(const Entity& other) const
		{
			return !(*this == other);
		}

		bool Valid() const
		{
			return m_handle != entt::null;
		}

		entt::entity GetHandle() const
		{
			BF_CORE_ASSERT(Valid(), "Entity::GetHandle: Entity is not valid");
			return m_handle;
		}

		template<typename T>
		T& AddComponent()
		{
			return Application::Get().GetScene().m_activeScene->AddComponent<T>(m_handle);
		}

		template<typename T>
		T& GetComponent()
		{
			return Application::Get().GetScene().m_activeScene->GetComponent<T>(m_handle);
		}

		template<typename T>
		const T& GetComponent() const
		{
			return Application::Get().GetScene().m_activeScene->GetComponent<T>(m_handle);
		}

		template<typename T>
		bool HasComponent() const
		{
			return Application::Get().GetScene().m_activeScene->HasComponent<T>(m_handle);
		}

		template<typename T>
		T* TryGetComponent()
		{
			return Application::Get().GetScene().m_activeScene->TryGetComponent<T>(m_handle);
		}

		template<typename T>
		void RemoveComponent()
		{
			Application::Get().GetScene().m_activeScene->RemoveComponent<T>(m_handle);
		}

		void Destroy()
		{
			Application::Get().GetScene().m_activeScene->DestroyEntity(m_handle);
		}

	private:
		friend class Scene;

		entt::entity m_handle = entt::null;
	};
}