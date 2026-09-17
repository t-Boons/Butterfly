#include "Core/Common.hpp"
#include "Serialization/ENTT/ComponentRegistry.hpp"
#include "Serialization/YAML/SerializeGlm.hpp"
#include "Serialization/YAML/SerializeUUID.hpp"

#include "Scene/Registry/TransformComponent.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"
#include "Scene/Registry/IDComponent.hpp"
#include "Scene/Registry/NameComponent.hpp"
#include "Scene/Registry/SkyboxComponent.hpp"

namespace Butterfly
{
	static ComponentRegistry g_initializer;

	void ComponentRegistry::RegisterComponents()
	{
		entt::meta_factory<NameComponent>{}
		.type("Name")
			.data<&NameComponent::Name>("Name")
			.data<&NameComponent::Tag>("Tag");

		s_components.push_back(CreateSerializer<NameComponent>());

		entt::meta_factory<IDComponent>{}
		.type("ID")
			.data<&IDComponent::EntityUUID>("EntityUUID");

		s_components.push_back(CreateSerializer<IDComponent>());

		entt::meta_factory<TransformComponent>{}
		.type("Transform")
			.data<&TransformComponent::SetPosition, &TransformComponent::GetPosition>("Position")
			.data<&TransformComponent::SetRotation, &TransformComponent::GetRotation>("Rotation")
			.data<&TransformComponent::SetScale, &TransformComponent::GetScale>("Scale")
			.data<&TransformComponent::SetChildrenUUIDs, &TransformComponent::GetChildrenUUIDs>("Children")
			.data<&TransformComponent::SetParentUUID, &TransformComponent::GetParentUUID>("Parent");
		s_components.push_back(CreateSerializer<TransformComponent>());

		entt::meta_factory<MeshRendererComponent>{}
		.type("Mesh")
			.data<&MeshRendererComponent::SetMeshUUID, &MeshRendererComponent::GetMeshUUID>("MeshUUID");

		s_components.push_back(CreateSerializer<MeshRendererComponent>());


		entt::meta_factory<SkyboxComponent>{}
		.type("Skybox")
			.data<&SkyboxComponent::SetTextureUUIDPositiveRight, &SkyboxComponent::GetTextureUUIDPositiveRight>("TextureUUIDPositiveRight")
			.data<&SkyboxComponent::SetTextureUUIDPositiveLeft, &SkyboxComponent::GetTextureUUIDPositiveLeft>("TextureUUIDPositiveLeft")
			.data<&SkyboxComponent::SetTextureUUIDPositiveTop, &SkyboxComponent::GetTextureUUIDPositiveTop>("TextureUUIDPositiveTop")
			.data<&SkyboxComponent::SetTextureUUIDPositiveBottom, &SkyboxComponent::GetTextureUUIDPositiveBottom>("TextureUUIDPositiveBottom")
			.data<&SkyboxComponent::SetTextureUUIDPositiveFront, &SkyboxComponent::GetTextureUUIDPositiveFront>("TextureUUIDPositiveFront")
			.data<&SkyboxComponent::SetTextureUUIDPositiveBack, &SkyboxComponent::GetTextureUUIDPositiveBack>("TextureUUIDPositiveBack");

		s_components.push_back(CreateSerializer<SkyboxComponent>());
	}

	YAML::Node ComponentRegistry::SerializeValue(const entt::meta_any& value)
	{
		if (value.type() == entt::resolve<glm::vec2>())
			return YAML::Node(value.cast<const glm::vec2&>());

		if (value.type() == entt::resolve<glm::vec3>())
			return YAML::Node(value.cast<const glm::vec3&>());

		if (value.type() == entt::resolve<glm::vec4>())
			return YAML::Node(value.cast<const glm::vec4&>());

		if (value.type() == entt::resolve<glm::quat>())
			return YAML::Node(value.cast<const glm::quat&>());

		if (value.type() == entt::resolve<float>())
			return YAML::Node(value.cast<float>());

		if (value.type() == entt::resolve<bool>())
			return YAML::Node(value.cast<bool>());

		if (value.type() == entt::resolve<int>())
			return YAML::Node(value.cast<int>());

		if (value.type() == entt::resolve<std::string>())
			return YAML::Node(value.cast<std::string>());

		if (value.type() == entt::resolve<Butterfly::UUID>())
			return YAML::Node(value.cast<Butterfly::UUID>());

		if (value.type() == entt::resolve<std::vector<Butterfly::UUID>>())
		{
			YAML::Node node(YAML::NodeType::Sequence);

			for (const UUID& uuid : value.cast<const std::vector<UUID>&>())
				node.push_back(uuid);

			return node;
		}

		BF_CORE_LOG_CRITICAL("Unsupported type for serialization: %s", value.type().info().name().data());
		return {};
	}

	bool ComponentRegistry::DeserializeValue(entt::meta_any& value, const YAML::Node& node)
	{
		if (value.type() == entt::resolve<glm::vec2>())
		{
			value.cast<glm::vec2&>() = node.as<glm::vec2>();
			return true;
		}

		if (value.type() == entt::resolve<glm::vec3>())
		{
			value.cast<glm::vec3&>() = node.as<glm::vec3>();
			return true;
		}

		if (value.type() == entt::resolve<glm::vec4>())
		{
			value.cast<glm::vec4&>() = node.as<glm::vec4>();
			return true;
		}

		if (value.type() == entt::resolve<glm::quat>())
		{
			value.cast<glm::quat&>() = node.as<glm::quat>();
			return true;
		}

		if (value.type() == entt::resolve<float>())
		{
			value.cast<float&>() = node.as<float>();
			return true;
		}

		if (value.type() == entt::resolve<bool>())
		{
			value.cast<bool&>() = node.as<bool>();
			return true;
		}

		if (value.type() == entt::resolve<int>())
		{
			value.cast<int&>() = node.as<int>();
			return true;
		}

		if (value.type() == entt::resolve<std::string>())
		{
			value.cast<std::string&>() = node.as<std::string>();
			return true;
		}

		if (value.type() == entt::resolve<Butterfly::UUID>())
		{
			value.cast<Butterfly::UUID&>() = node.as<Butterfly::UUID>();
			return true;
		}

		if (value.type() == entt::resolve<std::vector<Butterfly::UUID>>())
		{
			auto& children = value.cast<std::vector<UUID>&>();

			children.clear();

			for (const auto& childNode : node)
				children.push_back(childNode.as<UUID>());

			return true;
		}

		BF_CORE_LOG_CRITICAL("Unsupported type for deserialization: %s", value.type().info().name().data());
		return false;
	}
}