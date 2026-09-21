#pragma once
#include "Scene/Entity.hpp"

namespace Butterfly
{
	class IDComponent
	{
	public:
		IDComponent(const Entity& entity)
		{
		}

		EntityUUID EntityUUID;
	};
}
