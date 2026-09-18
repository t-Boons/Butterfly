#pragma once
#include "Butterfly.hpp"

namespace Butterfly
{
	struct EditorCacheFileMetadata
	{
		UUID ID;
		std::string CachedDataType;
		std::vector<uint8_t> Data;
	};
}

namespace YAML
{
	template<>
	struct convert<Butterfly::EditorCacheFileMetadata>
	{
		static Node encode(const Butterfly::EditorCacheFileMetadata& metadata)
		{
			Node node;

			node["CachedDataType"] = metadata.CachedDataType;
			node["ID"] = metadata.ID.ToString();
			node["Data"] = YAML::Binary(metadata.Data.data(), metadata.Data.size());

			return node;
		}

		static bool decode(const Node& node, Butterfly::EditorCacheFileMetadata& metadata)
		{
			if (!node.IsMap())
			{
				return false;
			}

			metadata.CachedDataType = node["CachedDataType"].as<std::string>();
			metadata.ID = Butterfly::UUID::FromString(node["ID"].as<std::string>());

			const auto binary = node["Data"].as<YAML::Binary>();
			metadata.Data.assign(binary.data(), binary.data() + binary.size());

			return true;
		}
	};
}