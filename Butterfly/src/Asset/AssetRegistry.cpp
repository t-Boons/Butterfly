#include "Asset/AssetRegistry.hpp"
#include "Asset/AssetManager.hpp"
#include "Core/UUID.hpp"
#include "Core/FileSystem.hpp"

namespace Butterfly
{
	AssetRegistry::AssetRegistry(AssetManager* manager)
	{
		m_manager = manager;
		s_assetPath = FileSystem::WorkingDirectory() / "Assets";

		Scan();
	}

	bool AssetRegistry::ImportFromDisk(const std::filesystem::path& file, AssetMetadata& meta)
	{
		if (!FileSystem::Copy(file, s_assetPath / file.filename()))
		{
			BF_CORE_LOG_ERROR("Unable to copy file to assets directory.");
			return false;
		}

		AssetMetadata newMeta = WriteNewMetaForFile(file);
		meta = newMeta;
		m_registeredAssets[meta.ID] = meta;

		return true;
	}

	bool AssetRegistry::Find(const UUID& id, AssetMetadata& out) const
	{
		auto it = m_registeredAssets.find(id);
		if (it == m_registeredAssets.end())
		{
			return false;
		}
		out = it->second;
		return true;
	}

	void AssetRegistry::Scan()
	{
		for (auto& file : FileSystem::WalkDirectoryRecursive(s_assetPath))
		{
			const std::string extention = file.extension().string();
			if (extention == ".meta")
			{
				continue;
			}

			const std::filesystem::path metaPath = file.string() + s_metaFileExtention;
			if (FileSystem::Exists(metaPath)) 
			{
				AssetMetadata meta = ReadMetaFromFile(file);
				m_registeredAssets[meta.ID] = meta;
			}
			else
			{
				AssetMetadata meta = WriteNewMetaForFile(file);
				m_registeredAssets[meta.ID] = meta;
			}
		}
	}

	AssetMetadata AssetRegistry::WriteNewMetaForFile(const std::filesystem::path& file) const
	{
		const std::filesystem::path metaPath = file.string() + s_metaFileExtention;

		AssetMetadata meta;
		meta.Path = file.string();
		meta.ID = UUID::Generate();
		meta.Extention = file.extension().string();

		YAML::Node node;
		node["Meta"] = meta;
		YAML::Emitter emitter;
		emitter << node;

		if (!FileSystem::WriteText(metaPath, emitter.c_str()))
		{
			BF_CORE_LOG_ERROR("Failed to create meta file for newly registered asset: %ls", metaPath.filename().c_str());
		}
		return meta;
	}

	AssetMetadata AssetRegistry::ReadMetaFromFile(const std::filesystem::path& file) const
	{
		const std::filesystem::path metaPath = file.string() + s_metaFileExtention;

		YAML::Node node;
		node = YAML::LoadFile(metaPath.string());
		return node["Meta"].as<AssetMetadata>();
	}
}