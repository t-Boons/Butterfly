#pragma once
#include "Core/UUID.hpp"

namespace Butterfly
{
	class IDComponent
	{
	public:
		IDComponent(const Entity& entity)
		{
		}

		UUID EntityUUID;
	};
}
