#pragma once
#include "Core/UUID.hpp"
#include "Asset/AssetUUID.hpp"
#include "Asset/AssetTypes.hpp"
#include "Scene/Entity.hpp"

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

	template<>
	struct convert<Butterfly::AssetUUID<Butterfly::TextureAsset>>
	{
		static Node encode(const Butterfly::AssetUUID<Butterfly::TextureAsset>& value)
		{
			Node node;
			node["TextureUUID"] = value.ToString();
			return node;
		}

		static bool decode(const Node& node, Butterfly::AssetUUID<Butterfly::TextureAsset>& value)
		{
			if (!node.IsMap() || !node["TextureUUID"])
				return false;

			value = Butterfly::UUID::FromString(node["TextureUUID"].as<std::string>());

			return true;
		}
	};

	template<>
	struct convert<Butterfly::AssetUUID<Butterfly::MeshAsset>>
	{
		static Node encode(const Butterfly::AssetUUID<Butterfly::MeshAsset>& value)
		{
			Node node;
			node["MeshUUID"] = value.ToString();
			return node;
		}

		static bool decode(const Node& node, Butterfly::AssetUUID<Butterfly::MeshAsset>& value)
		{
			if (!node.IsMap() || !node["MeshUUID"])
				return false;

			value = Butterfly::UUID::FromString(node["MeshUUID"].as<std::string>());

			return true;
		}
	};

	template<>
	struct convert<Butterfly::EntityUUID>
	{
		static Node encode(const Butterfly::EntityUUID& value)
		{
			Node node;
			node["EntityUUID"] = value.ToString();
			return node;
		}

		static bool decode(const Node& node, Butterfly::EntityUUID& value)
		{
			if (!node.IsMap() || !node["EntityUUID"])
				return false;

			value = Butterfly::UUID::FromString(node["EntityUUID"].as<std::string>());

			return true;
		}
	};
}