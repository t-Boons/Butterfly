#pragma once
#include "Asset/AssetHandle.hpp"

namespace YAML
{
	template<typename T>
	struct convert<Butterfly::AssetHandle<T>>
	{
		static Node encode(const Butterfly::AssetHandle<T>& value)
		{
			Node node;
			node["UUID"] = value.GetID().ToString();
			return node;
		}

		static bool decode(const Node& node, Butterfly::AssetHandle<T>& value)
		{
			if (!node.IsMap() || !node["UUID"])
				return false;

			value = Butterfly::UUID::FromString(node["UUID"].as<std::string>());

			return true;
		}
	};
}