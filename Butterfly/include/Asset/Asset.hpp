#pragma once
#include "Core/Common.hpp"
#include "Core/UUID.hpp"
#include <yaml-cpp/yaml.h>

namespace Butterfly
{
	template<typename T>
	class AssetHandle
	{
	public:
		UUID ID;
	};

	struct AssetType
	{
		std::string TypeName;
	};

	struct AssetMetadata
	{
		std::string Path;
		std::string Extention;
		AssetType Type;
		UUID ID;
	};

	struct AssetEntry
	{
		UUID ID;
		AssetType Type;
		RefPtr<void> Data;
		uint32_t RefCount = 0;
	};
}

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

