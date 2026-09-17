#pragma once

namespace Butterfly
{
	// Only used for destruction of entities.
	class Entity;
	class PendingDestroyComponent
	{
		PendingDestroyComponent(const Entity& entity)
		{
		}
	};
}