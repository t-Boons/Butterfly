#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"
#include "Core/Application.hpp"

namespace Butterfly
{
	Entity::Entity(EntityHandle handle)
		: m_registry(&Application::Get().GetScene().GetEntityRegistry()), m_handle(handle)
	{

	}
}