#include "Scene/Entity.hpp"

namespace Butterfly
{
	Entity::Entity()
		: m_handle(entt::null)
	{
	}

	Entity::Entity(entt::entity handle)
		: m_handle(handle)
	{

	}
}