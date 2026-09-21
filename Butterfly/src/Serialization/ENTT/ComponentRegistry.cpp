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

		if (value.type() == entt::resolve<std::string>())
		{
			node = YAML::Node(value.cast<std::string>());
			return true;
		}

		if (value.type() == entt::resolve<Butterfly::EntityUUID>())
		{
			node = YAML::Node(value.cast<Butterfly::EntityUUID>());
			return true;
		}

		if (value.type() == entt::resolve<Butterfly::AssetUUID<TextureAsset>>())
		{
			node = YAML::Node(value.cast<Butterfly::AssetUUID<TextureAsset>>());
			return true;
		}

		if (value.type() == entt::resolve<Butterfly::AssetUUID<MeshAsset>>())
		{
			node = YAML::Node(value.cast<Butterfly::AssetUUID<MeshAsset>>());
			return true;
		}

		if (value.type() == entt::resolve<std::vector<Butterfly::AssetUUID<TextureAsset>>>())
		{
			node = YAML::Node(YAML::NodeType::Sequence);

			for (const AssetUUID<TextureAsset>& uuid : value.cast<const std::vector<AssetUUID<TextureAsset>>&>())
				node.push_back(uuid);

			return true;
		}

		if (value.type() == entt::resolve<std::vector<Butterfly::AssetUUID<MeshAsset>>>())
		{
			node = YAML::Node(YAML::NodeType::Sequence);

			for (const AssetUUID<MeshAsset>& uuid : value.cast<const std::vector<AssetUUID<MeshAsset>>&>())
				node.push_back(uuid);

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

		if (value.type() == entt::resolve<std::string>())
		{
			value.cast<std::string&>() = node.as<std::string>();
			return true;
		}

		if (value.type() == entt::resolve<Butterfly::AssetUUID<TextureAsset>>())
		{
			value.cast<Butterfly::AssetUUID<TextureAsset>&>() = node.as<Butterfly::AssetUUID<TextureAsset>>();
			return true;
		}

		if (value.type() == entt::resolve<Butterfly::AssetUUID<MeshAsset>>())
		{
			value.cast<Butterfly::AssetUUID<MeshAsset>&>() = node.as<Butterfly::AssetUUID<MeshAsset>>();
			return true;
		}

		if (value.type() == entt::resolve<std::vector<Butterfly::AssetUUID<TextureAsset>>>())
		{
			auto& children = value.cast<std::vector<Butterfly::AssetUUID<TextureAsset>>&>();

			children.clear();

			for (const auto& childNode : node)
				children.push_back(childNode.as<Butterfly::AssetUUID<TextureAsset>>());

			return true;
		}

		if (value.type() == entt::resolve<std::vector<Butterfly::AssetUUID<MeshAsset>>>())
		{
			auto& children = value.cast<std::vector<Butterfly::AssetUUID<MeshAsset>>&>();

			children.clear();

			for (const auto& childNode : node)
				children.push_back(childNode.as<Butterfly::AssetUUID<MeshAsset>>());

			return true;
		}
		return false;
	}
}