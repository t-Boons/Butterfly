#include "Core/Common.hpp"
#include "Serialization/ENTT/ComponentRegistry.hpp"
#include "Serialization/YAML/SerializeGlm.hpp"
#include "Serialization/YAML/SerializeUUID.hpp"

namespace Butterfly
{
	static ComponentRegistry g_initializer;

	bool ComponentRegistry::SerializeValue(const entt::meta_any& value, YAML::Node& node)
	{
		if (value.type() == entt::resolve<glm::vec2>())
		{
			node = YAML::Node(value.cast<const glm::vec2&>());
			return true;
		}

		if (value.type() == entt::resolve<glm::vec3>())
		{
			node = YAML::Node(value.cast<const glm::vec3&>());
			return true;
		}

		if (value.type() == entt::resolve<glm::vec4>())
		{
			node = YAML::Node(value.cast<const glm::vec4&>());
			return true;
		}

		if (value.type() == entt::resolve<glm::quat>())
		{
			node = YAML::Node(value.cast<const glm::quat&>());
			return true;
		}

		if (value.type() == entt::resolve<float>())
		{
			node = YAML::Node(value.cast<float>());
			return true;
		}

		if (value.type() == entt::resolve<bool>())
		{
			node = YAML::Node(value.cast<bool>());
			return true;
		}

		if (value.type() == entt::resolve<int>())
		{
			node = YAML::Node(value.cast<int>());
			return true;
		}

		if (value.type() == entt::resolve<uint32_t>())
		{
			node = YAML::Node(value.cast<uint32_t>());
			return true;
		}

		if (value.type() == entt::resolve<uint64_t>())
		{
			node = YAML::Node(value.cast<uint64_t>());
			return true;
		}

		if (value.type() == entt::resolve<std::string>())
		{
			node = YAML::Node(value.cast<std::string>());
			return true;
		}

		if (value.type() == entt::resolve<Butterfly::UUID>())
		{
			node = YAML::Node(value.cast<Butterfly::UUID>());
			return true;
		}

		if (value.type() == entt::resolve<std::vector<Butterfly::UUID>>())
		{
			node = YAML::Node(YAML::NodeType::Sequence);

			for (const UUID& uuid : value.cast<const std::vector<UUID>&>())
			{
				node.push_back(uuid);
			}

			return true;
		}


		return false;
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

		if (value.type() == entt::resolve<uint32_t>())
		{
			value.cast<uint32_t&>() = node.as<uint32_t>();
			return true;
		}

		if (value.type() == entt::resolve<uint64_t>())
		{
			value.cast<uint64_t&>() = node.as<uint64_t>();
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
			std::vector<Butterfly::UUID>& vec = value.cast<std::vector<Butterfly::UUID>&>();
			vec.clear();
			for (const YAML::Node& uuidNode : node)
			{
				vec.push_back(uuidNode.as<Butterfly::UUID>());
			}
			return true;
		}

		return false;
	}
}