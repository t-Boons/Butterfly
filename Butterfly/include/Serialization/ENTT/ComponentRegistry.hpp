#pragma once
#include "Core/Common.hpp"
#include "Scene/Entity.hpp"

#include "Scene/Registry/TransformComponent.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"
#include "Scene/Registry/IDComponent.hpp"
#include "Scene/Registry/NameComponent.hpp"
#include "Scene/Registry/SkyboxComponent.hpp"

namespace Butterfly
{
	class ComponentRegistry
	{
	public:

		struct ComponentSerializer
		{
			entt::id_type TypeID;
			std::string_view Name;

			std::function<bool(entt::registry&, entt::entity)> Has;
			std::function<void(YAML::Node&, entt::registry&, entt::entity)> Serialize;
			std::function<bool(const YAML::Node&, entt::registry&, entt::entity)> Deserialize;
			std::function<void(const entt::registry& source, entt::registry& target, entt::entity src, entt::entity dst)> CopyEntityComponents;
		};

		ComponentRegistry()
		{
			RegisterComponents();
		}


		using Components = std::tuple<
			NameComponent,
			IDComponent,
			TransformComponent,
			MeshRendererComponent,
			SkyboxComponent>;

		static void RegisterComponents()
		{
			entt::meta_factory<NameComponent>{}
			.type("Name")
				.data<&NameComponent::Name>("Name")
				.data<&NameComponent::Tag>("Tag");

			entt::meta_factory<IDComponent>{}
			.type("ID")
				.data<&IDComponent::EntityUUID>("EntityUUID");


			entt::meta_factory<TransformComponent>{}
			.type("Transform")
				.data<&TransformComponent::SetPosition, &TransformComponent::GetPosition>("Position")
				.data<&TransformComponent::SetRotation, &TransformComponent::GetRotation>("Rotation")
				.data<&TransformComponent::SetScale, &TransformComponent::GetScale>("Scale")
				.data<&TransformComponent::SetChildrenUUIDs, &TransformComponent::GetChildrenUUIDs>("Children")
				.data<&TransformComponent::SetParentUUID, &TransformComponent::GetParentUUID>("Parent");

			entt::meta_factory<MeshRendererComponent>{}
			.type("Mesh")
				.data<&MeshRendererComponent::SetMeshUUID, &MeshRendererComponent::GetMeshUUID>("MeshUUID");


			entt::meta_factory<SkyboxComponent>{}
			.type("Skybox")
				.data<&SkyboxComponent::SetTextureUUIDPositiveRight, &SkyboxComponent::GetTextureUUIDPositiveRight>("TextureUUIDPositiveRight")
				.data<&SkyboxComponent::SetTextureUUIDPositiveLeft, &SkyboxComponent::GetTextureUUIDPositiveLeft>("TextureUUIDPositiveLeft")
				.data<&SkyboxComponent::SetTextureUUIDPositiveTop, &SkyboxComponent::GetTextureUUIDPositiveTop>("TextureUUIDPositiveTop")
				.data<&SkyboxComponent::SetTextureUUIDPositiveBottom, &SkyboxComponent::GetTextureUUIDPositiveBottom>("TextureUUIDPositiveBottom")
				.data<&SkyboxComponent::SetTextureUUIDPositiveFront, &SkyboxComponent::GetTextureUUIDPositiveFront>("TextureUUIDPositiveFront")
				.data<&SkyboxComponent::SetTextureUUIDPositiveBack, &SkyboxComponent::GetTextureUUIDPositiveBack>("TextureUUIDPositiveBack");


			RunOnAllComponents([&]<typename T>()
			{
				s_components.push_back(CreateSerializer<T>());
			});

		}

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
				{
					continue;
				}

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

		template<typename Func>
		static void RunOnAllComponents(Func&& func)
		{
			RunOnAllComponentsImpl(std::forward<Func>(func), static_cast<Components*>(nullptr));
		}

		static void SerializeComponents(YAML::Node& node, const entt::registry& registry, entt::entity entity)
		{
			RunOnAllComponents([&]<typename T>()
			{
				if (!registry.all_of<T>(entity))
				{
					return;
				}

				node.push_back(SerializeComponent(registry.get<T>(entity)));
			});
		}

		static void DeserializeComponents(const YAML::Node& node, entt::registry& registry, entt::entity entity)
		{
			RunOnAllComponents([&]<typename T>()
			{
				const std::string_view componentName = entt::resolve<T>().name();

				for (const auto& componentNode : node)
				{
					if (!componentNode[componentName])
					{
						continue;
					}

					T& component = registry.emplace<T>(entity, Entity(entity));
					DeserializeComponent(componentNode, component);
					break;
				}
			});
		}


	private:

		template<typename Func, typename... Ts>
		static void RunOnAllComponentsImpl(Func&& func, std::tuple<Ts...>*)
		{
			(std::forward<Func>(func).template operator() < Ts > (), ...);
		}

		template<typename T>
		static ComponentSerializer CreateSerializer()
		{
			ComponentSerializer serializer;

			const entt::meta_type type = entt::resolve<T>();
			serializer.Name = type.name();
			serializer.TypeID = entt::type_hash<T>::value();
			serializer.Has = [](entt::registry& registry, entt::entity entity)
				{
					return registry.all_of<T>(entity);
				};

			BF_CORE_LOG_INFO("Registered component serializer: %s", serializer.Name.data());
			return serializer;
		}

		static YAML::Node SerializeValue(const entt::meta_any& value);
		static bool DeserializeValue(entt::meta_any& value, const YAML::Node& node);

		inline static std::vector<ComponentSerializer> s_components;
	};
}