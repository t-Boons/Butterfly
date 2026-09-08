#pragma once
#include "Core/Common.hpp"
#include "Core/UUID.hpp"
#include "Asset/Importer/AssetImporter.hpp"
#include "Core/FileSystem.hpp"
#include <yaml-cpp/yaml.h>

namespace Butterfly
{
	struct AssetMetadata
	{
		std::string Path;
		std::string Type;
		UUID ID;
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
			node["Type"] = meta.Type;
			node["ID"] = meta.ID.ToString();

			return node;
		}

		static bool decode(const Node& node, Butterfly::AssetMetadata& meta)
		{
			if (!node.IsMap())
			{
				return false;
			}

			meta.Path = node["Path"].as<std::string>();
			meta.Type = node["Type"].as<std::string>();
			meta.ID = Butterfly::UUID::FromString(node["ID"].as<std::string>());

			return true;
		}
	};
}


namespace Butterfly
{
	class AssetRegistry : public NonCopyableNonMoveable
	{
	public:
		AssetRegistry()
		{
			m_importers = IAssetImporter::CreateImporters();
			s_assetPath = FileSystem::WorkingDirectory() / "Assets";
		}

		bool Register(const std::filesystem::path& path, AssetMetadata& meta)
		{
			RefPtr<IAssetImporter> correctImporter = nullptr;
			for (auto& importer : m_importers)
			{
				if (importer->IsCorrectFileExtention(path.extension().string()))
				{
					correctImporter = importer;
				}
			}

			if (!correctImporter)
			{
				BF_CORE_LOG_WARN("File: %s has an unsupported extention.", path.c_str());
				return false;
			}

			if (!FileSystem::Copy(path, s_assetPath))
			{
				BF_CORE_LOG_ERROR("Unable to copy file to assets directory.");
				return false;
			}

			const std::filesystem::path filePath = s_assetPath / path.filename();

			meta.Path = filePath.string();
			meta.Type = correctImporter->AssetType();
			meta.ID = UUID::Generate();
			
			YAML::Node node;
			node["Meta"] = meta;
			YAML::Emitter emitter;
			emitter << node;
			
			if (!FileSystem::WriteText(FileSystem::ReplaceExtention(filePath, s_metaFileExtention), emitter.c_str()))
			{
				BF_CORE_LOG_ERROR("Failed to create meta file for newly registered asset: %ls", path.filename().c_str());
				return false;
			}

			return true;
		}

	private:
		std::vector<RefPtr<IAssetImporter>> m_importers;
		inline static std::filesystem::path s_assetPath;
		inline static const std::string s_metaFileExtention = ".meta";
	};
}