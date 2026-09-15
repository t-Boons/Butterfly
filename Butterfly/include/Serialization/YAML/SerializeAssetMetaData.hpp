#pragma once
#include "Asset/Asset.hpp"

namespace YAML
{
	template<>
	struct convert<Butterfly::AssetMetadata>
	{
		static Node encode(const Butterfly::AssetMetadata& meta)
		{
			Node node;

			node["Path"] = meta.Path;
			node["Type"] = meta.Type.TypeName;
			node["ID"] = meta.ID.ToString();
			node["Extention"] = meta.Extention;

			return node;
		}

		static bool decode(const Node& node, Butterfly::AssetMetadata& meta)
		{
			if (!node.IsMap())
			{
				return false;
			}

			meta.Path = node["Path"].as<std::string>();
			meta.Type.TypeName = node["Type"].as<std::string>();
			meta.ID = Butterfly::UUID::FromString(node["ID"].as<std::string>());
			meta.Extention = node["Extention"].as<std::string>();

			return true;
		}
	};
}