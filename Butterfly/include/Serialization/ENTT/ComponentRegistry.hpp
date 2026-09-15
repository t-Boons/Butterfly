#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
	struct ComponentSerializer
	{
		entt::id_type TypeID;
		const char* Name;

		std::function<bool(entt::registry&, entt::entity)> Has;
		std::function<void(YAML::Node&, entt::registry&, entt::entity)> Serialize;
	};	

	class ComponentRegistry
	{
	public:
		ComponentRegistry()
		{
			RegisterComponents();
		}

		static void RegisterComponents();

		static const std::vector<ComponentSerializer>& GetComponentSerializers() { return s_components; }

		template<typename T>
		static YAML::Node SerializeComponent(const T& component)
		{
			YAML::Node componentNode;
			auto type = entt::resolve<T>();

			for (auto [id, data] : type.data())
			{
				auto value = data.get(component);

				if (!value)
					continue;

				componentNode[data.name()] = SerializeValue(value);
			}

			YAML::Node node;
			node[type.name()] = componentNode;

			return node;
		}

		template<typename T>
		static bool DeserializeComponent(const YAML::Node& node, T& component)
		{
			auto type = entt::resolve<T>();
			YAML::Node componentNode = node[type.name()];

			for (auto [id, data] : type.data())
			{
				YAML::Node fieldNode = componentNode[data.name()];

				if (!fieldNode)
					continue;

				auto field = data.get(component);

				if (!field)
					continue;

				DeserializeValue(field, fieldNode);
				data.set(component, field);
			}

			return true;
		}

	private:

		template<typename T>
		static ComponentSerializer CreateSerializer()
		{
			ComponentSerializer serializer;
			serializer.Name = entt::type_name<T>::value().data();
			serializer.TypeID = entt::type_hash<T>::value();
			serializer.Has = [](entt::registry& registry, entt::entity entity)
				{
					return registry.all_of<T>(entity);
				};
			serializer.Serialize = [](YAML::Node& node, entt::registry& registry, entt::entity entity)
				{
					const T& component = registry.get<T>(entity);
					node = SerializeComponent<T>(component);
				};

			entt::meta_type type = entt::resolve<T>();
			auto name = type.info();
			BF_CORE_LOG_INFO("Registered componefnt serializer: %s", name.name().data());
			return serializer;
		}

		static YAML::Node SerializeValue(const entt::meta_any& value);
		static bool DeserializeValue(entt::meta_any& value, const YAML::Node& node);

		inline static std::vector<ComponentSerializer> s_components;
	};
}