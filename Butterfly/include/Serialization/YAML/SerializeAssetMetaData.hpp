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

			node["SourceFileID"] = meta.SourceFileID.ToString();
			node["Name"] = meta.Name;
			node["Type"] = meta.Type.TypeName;
			node["AssetID"] = meta.AssetID.ToString();

			return node;
		}

		static bool decode(const Node& node, Butterfly::AssetMetadata& meta)
		{
			if (!node.IsMap())
			{
				return false;
			}

			meta.SourceFileID = Butterfly::UUID::FromString(node["SourceFileID"].as<std::string>());
			meta.Name = node["Name"].as<std::string>();
			meta.Type.TypeName = node["Type"].as<std::string>();
			meta.AssetID = Butterfly::UUID::FromString(node["AssetID"].as<std::string>());

			return true;
		}
	};

	template<>
	struct convert<Butterfly::AssetFileMetadata>
	{
		static Node encode(const Butterfly::AssetFileMetadata& meta)
		{
			Node node;

			node["Path"] = meta.Path.string();
			node["SourceFileID"] = meta.SourceFileID.ToString();
			node["RootAssetID"] = meta.RootAssetID.ToString();

			for (auto& subAsset : meta.Assets)
			{
				node["Assets"].push_back(subAsset.second);
			}

			return node;
		}

		static bool decode(const Node& node, Butterfly::AssetFileMetadata& meta)
		{
			if (!node.IsMap())
			{
				return false;
			}

			meta.Path = std::filesystem::path(node["Path"].as<std::string>());
			meta.SourceFileID = Butterfly::UUID::FromString(node["SourceFileID"].as<std::string>());
			meta.RootAssetID = Butterfly::UUID::FromString(node["RootAssetID"].as<std::string>());

			for (auto& subAssetNode : node["Assets"])
			{
				if (subAssetNode.IsMap())
				{
					Butterfly::AssetMetadata subAsset;
					if (YAML::convert<Butterfly::AssetMetadata>::decode(subAssetNode, subAsset))
					{
						meta.Assets.insert({ subAsset.AssetID, subAsset });
					}
				}
			}

			return true;
		}
	};
}