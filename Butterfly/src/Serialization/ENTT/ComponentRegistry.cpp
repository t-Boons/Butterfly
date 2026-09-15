#include "Core/Common.hpp"
#include "Serialization/ENTT/ComponentRegistry.hpp"
#include "Serialization/YAML/SerializeGlm.hpp"
#include "Serialization/YAML/SerializeUUID.hpp"

#include "Scene/Registry/TransformComponent.hpp"
#include "Scene/Registry/MeshRendererComponent.hpp"

namespace Butterfly
{
	static ComponentRegistry g_initializer;

	void ComponentRegistry::RegisterComponents()
	{
		entt::meta_factory<TransformComponent>{}
		.type("Transform")
			.data<&TransformComponent::m_position>("Position")
			.data<&TransformComponent::m_rotation>("Rotation")
			.data<&TransformComponent::m_scale>("Scale")
			.data<&TransformComponent::m_childUUID>("Child");

		s_components.push_back(CreateSerializer<TransformComponent>());

		entt::meta_factory<MeshRendererComponent>{}
		.type("MeshRenderer")
			.data<&MeshRendererComponent::SetSerializeID, &MeshRendererComponent::GetSerializeID>("MeshRenderer");

		s_components.push_back(CreateSerializer<MeshRendererComponent>());
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

		BF_CORE_LOG_CRITICAL("Unsupported type for deserialization: %s", value.type().info().name().data());
		return false;
	}
}