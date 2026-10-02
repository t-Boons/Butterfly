#pragma once
#include "Core/Common.hpp"
#include "Scene/Entity.hpp"

#include "Scene/Registry/TransformComponent.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"
#include "Scene/Registry/IDComponent.hpp"
#include "Scene/Registry/NameComponent.hpp"
#include "Scene/Registry/SkyboxComponent.hpp"
#include "Scene/Registry/LightComponent.hpp"
#include "Scene/Registry/CameraComponent.hpp"

namespace Butterfly
{
	template<typename T>
	class InspectPropertyWithCondition
	{
	public:
		InspectPropertyWithCondition(std::function<bool(const T& component)> condition)
			: m_condition(condition)
		{
		}

		bool Condition(const T& component) const
		{
			return m_condition(component);
		}

	private:
		std::function<bool(const T& component)> m_condition;
	};


	class InspectProperty
	{
	};

	class InspectComponent
	{
	};

	class NonDeletableComponent
	{
	};

	class Serializable
	{
	};

	class AsSlider
	{
	public:
		AsSlider(float min, float max)
			: m_min(min), m_max(max)
		{
		}

		float GetMin() const { return m_min; }
		float GetMax() const { return m_max; }
			
	private:
		float m_min;
		float m_max;
	};

	class AsColor
	{
	};

	class AsAssetSelector
	{
	public:
		AsAssetSelector(AssetType type)
			: Type(type)
		{
		}

		AssetType Type;
	};

	class AsEnumSelector
	{
	public:
		AsEnumSelector(std::vector<std::string> names)
			: m_enumNames(names)
		{
		}

		const std::vector<std::string>& GetEnumNames() const
		{
			return m_enumNames;
		}

	private:
		std::vector<std::string> m_enumNames;
	};

	class ComponentProperties
	{
	public:
		template<typename... T>
		ComponentProperties(T... properties)
		{
			(SetProperty(properties), ...);
		}

		template<typename T>
		T* TryGetProperty()
		{
			auto it = m_properties.find(typeid(T));

			if (it != m_properties.end())
			{
				return std::any_cast<T>(&it->second);
			}

			return nullptr;
		}

		template<typename T>
		const T* TryGetProperty() const
		{
			auto it = m_properties.find(typeid(T));

			if (it != m_properties.end())
			{
				return std::any_cast<T>(&it->second);
			}

			return nullptr;
		}

		template<typename T>
		void SetProperty(const T& value)
		{
			m_properties[typeid(T)] = value;
		}

	private:
		std::unordered_map<std::type_index, std::any> m_properties;
	};

	inline ComponentProperties& GetProperties(entt::meta_custom custom)
	{
		return *custom.operator ComponentProperties * ();
	}

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
			SkyboxComponent,
			LightComponent,
			CameraComponent>;

		static void RegisterComponents()
		{
			entt::meta_factory<NameComponent>{}
			.type("Name").custom<ComponentProperties>(Serializable{}, InspectComponent{}, NonDeletableComponent{})
				.data<&NameComponent::Name>("Name").custom<ComponentProperties>(Serializable{}, InspectProperty{})
				.data<&NameComponent::Tag>("Tag").custom<ComponentProperties>(Serializable{}, InspectProperty{});

			entt::meta_factory<IDComponent>{}
			.type("ID").custom<ComponentProperties>(Serializable{})
				.data<&IDComponent::EntityUUID>("EntityUUID").custom<ComponentProperties>(Serializable{});


			entt::meta_factory<TransformComponent>{}
			.type("Transform").custom<ComponentProperties>(Serializable{}, InspectComponent{}, NonDeletableComponent{})
				.data<&TransformComponent::SetPosition, &TransformComponent::GetPosition>("Position").custom<ComponentProperties>(Serializable{}, InspectProperty{})
				.data<&TransformComponent::SetRotation, &TransformComponent::GetRotation>("Rotation").custom<ComponentProperties>(Serializable{}, InspectProperty{})
				.data<&TransformComponent::SetScale, &TransformComponent::GetScale>("Scale").custom<ComponentProperties>(Serializable{}, InspectProperty{})
				.data<&TransformComponent::SetChildrenUUIDs, &TransformComponent::GetChildrenUUIDs>("Children").custom<ComponentProperties>(Serializable{})
				.data<&TransformComponent::SetParentUUID, &TransformComponent::GetParentUUID>("Parent").custom<ComponentProperties>(Serializable{});

			entt::meta_factory<MeshRendererComponent>{}
			.type("Mesh").custom<ComponentProperties>(Serializable{}, InspectComponent{})
				.data<&MeshRendererComponent::SetMeshUUID, &MeshRendererComponent::GetMeshUUID>("MeshUUID").custom<ComponentProperties>(Serializable{}, InspectProperty{}, AsAssetSelector{MeshAsset::Type()});

			entt::meta_factory<SkyboxComponent>{}
			.type("Skybox").custom<ComponentProperties>(Serializable{}, InspectComponent{})
				.data<&SkyboxComponent::SetTypeUInt, &SkyboxComponent::GetTypeUInt>("Type").custom<ComponentProperties>(Serializable{}, InspectProperty{}, AsEnumSelector{ {"Equirectangular", "Cubemap"} })
				.data<&SkyboxComponent::SetTextureUUIDRight, &SkyboxComponent::GetTextureUUIDRight>("TextureUUIDRight").custom<ComponentProperties>(Serializable{}, InspectPropertyWithCondition<SkyboxComponent>{[](const SkyboxComponent& component) { return component.GetType() == SkyboxType::Cubemap; }}, AsAssetSelector{TextureAsset::Type()})
				.data<&SkyboxComponent::SetTextureUUIDLeft, &SkyboxComponent::GetTextureUUIDLeft>("TextureUUIDLeft").custom<ComponentProperties>(Serializable{}, InspectPropertyWithCondition<SkyboxComponent>{[](const SkyboxComponent& component) { return component.GetType() == SkyboxType::Cubemap; }}, AsAssetSelector{TextureAsset::Type()})
				.data<&SkyboxComponent::SetTextureUUIDTop, &SkyboxComponent::GetTextureUUIDTop>("TextureUUIDTop").custom<ComponentProperties>(Serializable{}, InspectPropertyWithCondition<SkyboxComponent>{[](const SkyboxComponent& component) { return component.GetType() == SkyboxType::Cubemap; }}, AsAssetSelector{TextureAsset::Type()})
				.data<&SkyboxComponent::SetTextureUUIDBottom, &SkyboxComponent::GetTextureUUIDBottom>("TextureUUIDBottom").custom<ComponentProperties>(Serializable{}, InspectPropertyWithCondition<SkyboxComponent>{[](const SkyboxComponent& component) { return component.GetType() == SkyboxType::Cubemap; }}, AsAssetSelector{TextureAsset::Type()})
				.data<&SkyboxComponent::SetTextureUUIDFront, &SkyboxComponent::GetTextureUUIDFront>("TextureUUIDFront").custom<ComponentProperties>(Serializable{}, InspectPropertyWithCondition<SkyboxComponent>{[](const SkyboxComponent& component) { return component.GetType() == SkyboxType::Cubemap; }}, AsAssetSelector{TextureAsset::Type()})
				.data<&SkyboxComponent::SetTextureUUIDBack, &SkyboxComponent::GetTextureUUIDBack>("TextureUUIDBack").custom<ComponentProperties>(Serializable{}, InspectPropertyWithCondition<SkyboxComponent>{[](const SkyboxComponent& component) { return component.GetType() == SkyboxType::Cubemap; }}, AsAssetSelector{TextureAsset::Type()})
				.data<&SkyboxComponent::SetTextureUUIDHDRI, &SkyboxComponent::GetTextureUUIDHDRI>("TextureUUIDHDRI").custom<ComponentProperties>(Serializable{}, InspectPropertyWithCondition<SkyboxComponent>{[](const SkyboxComponent& component) { return component.GetType() == SkyboxType::Equirectangular; }}, AsAssetSelector{ TextureAsset::Type() });

			entt::meta_factory<LightComponent>{}
			.type("Light").custom<ComponentProperties>(Serializable{}, InspectComponent{})
				.data<&LightComponent::SetTypeAsUInt, &LightComponent::GetTypeAsUInt>("Type").custom<ComponentProperties>(Serializable{}, InspectProperty{}, AsEnumSelector{ {"Directional", "Point", "Spot"} })
				.data<&LightComponent::SetColor, &LightComponent::GetColor>("Color").custom<ComponentProperties>(Serializable{}, InspectProperty{}, AsColor{})
				.data<&LightComponent::SetRange, &LightComponent::GetRange>("Range").custom<ComponentProperties>(Serializable{}, InspectPropertyWithCondition<LightComponent>{[](const LightComponent& component) { return component.GetType() == LightType::Point || component.GetType() == LightType::Spot; }})
				.data<&LightComponent::SetInnerConeAngle, &LightComponent::GetInnerConeAngle>("InnerConeAngle").custom<ComponentProperties>(Serializable{}, InspectPropertyWithCondition<LightComponent>{[](const LightComponent& component) { return component.GetType() == LightType::Spot; }}, AsSlider{0.0f, 180.0f})
				.data<&LightComponent::SetOuterConeAngle, &LightComponent::GetOuterConeAngle>("OuterConeAngle").custom<ComponentProperties>(Serializable{}, InspectPropertyWithCondition<LightComponent>{[](const LightComponent& component) { return component.GetType() == LightType::Spot; }}, AsSlider{0.0f, 180.0f});

			entt::meta_factory<CameraComponent>{}
			.type("Camera").custom<ComponentProperties>(Serializable{}, InspectComponent{})
				.data<&CameraComponent::SetProjectionTypeAsUInt, &CameraComponent::GetProjectionTypeAsUInt>("ProjectionType").custom<ComponentProperties>(Serializable{}, InspectProperty{}, AsEnumSelector{ {"Perspective", "Orthographic"} })
				.data<&CameraComponent::SetFov, &CameraComponent::GetFov>("Fov").custom<ComponentProperties>(Serializable{}, InspectPropertyWithCondition<CameraComponent>{[](const CameraComponent& component) { return component.GetProjectionType() == CameraProjectionType::Perspective; }}, AsSlider{0.0f, 180.0f})
				.data<&CameraComponent::SetSize, &CameraComponent::GetSize>("Size").custom<ComponentProperties>(Serializable{}, InspectPropertyWithCondition<CameraComponent>{[](const CameraComponent& component) { return component.GetProjectionType() == CameraProjectionType::Orthographic; }})
				.data<&CameraComponent::SetZNear, &CameraComponent::GetZNear>("ZNear").custom<ComponentProperties>(Serializable{}, InspectProperty{})
				.data<&CameraComponent::SetZFar, &CameraComponent::GetZFar>("ZFar").custom<ComponentProperties>(Serializable{}, InspectProperty{})
				.data<&CameraComponent::SetCameraIndex, &CameraComponent::GetCameraIndex>("CameraIndex").custom<ComponentProperties>(Serializable{}, InspectProperty{});
				
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


				YAML::Node valueNode;
				if (SerializeValue(value, valueNode))
				{
					componentNode[data.name()] = valueNode;
				}
				else
				{
					BF_CORE_LOG_CRITICAL("Failed to serialize value: %s", data.name().data());
				}
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

				if(DeserializeValue(field, fieldNode))
				{
					data.set(component, field);
				}
				else
				{
					BF_CORE_LOG_CRITICAL("Failed to deserialize value: %s on component %s", data.name().data(), type.name().data());
				}
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

		static bool SerializeValue(const entt::meta_any& value, YAML::Node& node);
		static bool DeserializeValue(entt::meta_any& value, const YAML::Node& node);

		inline static std::vector<ComponentSerializer> s_components;
	};
}