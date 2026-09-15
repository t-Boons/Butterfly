#pragma once
#include "Core/UUID.hpp"

namespace YAML
{
	template<>
	struct convert<Butterfly::UUID>
	{
		static Node encode(const Butterfly::UUID& value)
		{
			Node node;
			node["UUID"] = value.ToString();
			return node;
		}

		static bool decode(const Node& node, Butterfly::UUID& value)
		{
			if (!node.IsMap() || !node["UUID"])
				return false;

			value = Butterfly::UUID::FromString(node["UUID"].as<std::string>());

			return true;
		}
	};
}