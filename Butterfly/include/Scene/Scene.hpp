#pragma once
#include "Core/Common.hpp"
#include "Scene/Entity.hpp"
#include "entt/entt.hpp"

namespace Butterfly
{
	class Scene : public NonCopyable
	{
	public:
		Scene();

		void Tick();

		YAML::Node Serialize();
		void Deserialize(const std::string& text);

		void DestroyPendingEntities();
		Entity CreateEntity();
		entt::registry& GetEntityRegistry() { return m_entityRegistry; }

	private:
		entt::registry m_entityRegistry;
	};
}