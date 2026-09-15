#include "Scene/Entity.hpp"
#include "Scene/Scene.hpp"
#include "Core/Application.hpp"

namespace Butterfly
{
	Entity::Entity()
		: m_registry(&Application::Get().GetScene().GetEntityRegistry())
	{
	}
}