#pragma once
#include "glm/glm.hpp"

namespace YAML
{
	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& value)
		{
			Node node;
			node["x"] = value.x;
			node["y"] = value.y;
			return node;
		}

		static bool decode(const Node& node, glm::vec2& value)
		{
			if (!node.IsMap() || !node["x"] || !node["y"])
				return false;

			value.x = node["x"].as<float>();
			value.y = node["y"].as<float>();

			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& value)
		{
			Node node;
			node["x"] = value.x;
			node["y"] = value.y;
			node["z"] = value.z;
			return node;
		}

		static bool decode(const Node& node, glm::vec3& value)
		{
			if (!node.IsMap() || !node["x"] || !node["y"] || !node["z"])
				return false;

			value.x = node["x"].as<float>();
			value.y = node["y"].as<float>();
			value.z = node["z"].as<float>();

			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& value)
		{
			Node node;
			node["x"] = value.x;
			node["y"] = value.y;
			node["z"] = value.z;
			node["w"] = value.w;
			return node;
		}

		static bool decode(const Node& node, glm::vec4& value)
		{
			if (!node.IsMap() || !node["x"] || !node["y"] || !node["z"] || !node["w"])
				return false;

			value.x = node["x"].as<float>();
			value.y = node["y"].as<float>();
			value.z = node["z"].as<float>();
			value.w = node["w"].as<float>();

			return true;
		}
	};

	template<>
	struct convert<glm::quat>
	{
		static Node encode(const glm::quat& value)
		{
			Node node;
			node["w"] = value.w;
			node["x"] = value.x;
			node["y"] = value.y;
			node["z"] = value.z;
			return node;
		}

		static bool decode(const Node& node, glm::quat& value)
		{
			if (!node.IsMap() || !node["w"] || !node["x"] || !node["y"] || !node["z"])
				return false;

			value.w = node["w"].as<float>();
			value.x = node["x"].as<float>();
			value.y = node["y"].as<float>();
			value.z = node["z"].as<float>();

			return true;
		}
	};
}