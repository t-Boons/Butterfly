#pragma once
#include "Core/Common.hpp"
#include "Scene/Entity.hpp"

namespace Butterfly
{
	class Scene : public NonCopyable
	{
	public:
		void Init();
		void Tick();

		void DestroyPendingEntities();
		Entity CreateEntity();
		entt::registry& GetEntityRegistry() { return m_entityRegistry; }

	private:
		entt::registry m_entityRegistry;
	};
}