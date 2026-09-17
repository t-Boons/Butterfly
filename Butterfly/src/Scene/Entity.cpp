#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"
#include "Core/Application.hpp"

namespace Butterfly
{
	Entity::Entity()
		: m_registry(nullptr), m_handle(entt::null)
	{
	}

	Entity::Entity(entt::registry* registry, EntityHandle handle)
		: m_registry(registry), m_handle(handle)
	{

	}
}