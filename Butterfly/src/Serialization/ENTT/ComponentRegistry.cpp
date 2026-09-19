#include "Core/Common.hpp"
#include "Serialization/ENTT/ComponentRegistry.hpp"
#include "Serialization/YAML/SerializeGlm.hpp"
#include "Serialization/YAML/SerializeUUID.hpp"

namespace Butterfly
{
	static ComponentRegistry g_initializer;

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