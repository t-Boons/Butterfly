#include "Asset/AssetRegistry.hpp"
#include "Asset/AssetManager.hpp"
#include "Core/UUID.hpp"
#include "Core/FileSystem.hpp"
#include "Serialization/YAML/SerializeAssetMetaData.hpp"

namespace Butterfly
{
	AssetRegistry::AssetRegistry(AssetManager* manager)
	{
		m_manager = manager;
		m_assetPath = FileSystem::WorkingDirectory() / "Assets";

		Scan();
	}

	void AssetRegistry::Register(const AssetFileMetadata& meta)
	{
		m_registeredFiles[meta.SourceFileID] = meta;
		for (auto& asset : meta.Assets)
		{
			m_registeredAssets[asset.first] = asset.second;
		}
	}

	bool AssetRegistry::NewFile(const std::string& name, const std::string& extention, const std::string& contents, AssetFileMetadata& meta)
	{
		std::string stem = name;

		while (FileSystem::Exists(m_assetPath / (stem + extention)))
		{
			stem = Utils::IterateDuplicateName(stem);
		}

		const std::string newName = stem + extention;

		if (FileSystem::WriteText(m_assetPath / newName, contents))
		{
			AssetFileMetadata newMeta = WriteNewMetaForFile(m_assetPath / newName);
			meta = newMeta;
			Register(meta);

			return true;
		}
		else
		{
			BF_CORE_LOG_ERROR("Unable to create new file in assets directory: %s", name.c_str());
			return false;
		}
	}

	bool AssetRegistry::ImportFromDisk(const std::filesystem::path& file, AssetFileMetadata& meta)
	{
		if (!FileSystem::Copy(file, m_assetPath / file.filename()))
		{
			BF_CORE_LOG_ERROR("Unable to copy file to assets directory: %s", file.filename().string().c_str());
			return false;
		}

		AssetFileMetadata newMeta = WriteNewMetaForFile(file);
		meta = newMeta;
		Register(meta);
		return true;
	}

	bool AssetRegistry::FindAsset(const UUID& id, AssetMetadata& out) const
	{
		if (!id)
		{
			return false;
		}

		auto it = m_registeredAssets.find(id);
		if (it != m_registeredAssets.end())
		{
			out = it->second;
			return true;
		}

		return false;
	}

	bool AssetRegistry::FindFile(const UUID& id, AssetFileMetadata& out) const
	{
		if (!id)
		{
			return false;
		}
		auto it = m_registeredFiles.find(id);
		if (it != m_registeredFiles.end())
		{
			out = it->second;
			return true;
		}
		return false;
	}

	void AssetRegistry::Scan()
	{
		for (auto& file : FileSystem::WalkDirectoryRecursive(m_assetPath))
		{
			const std::string extention = file.extension().string();
			if (extention == ".meta")
			{
				continue;
			}

			const std::filesystem::path metaPath = file.string() + m_metaFileExtention;
			if (FileSystem::Exists(metaPath)) 
			{
				AssetFileMetadata meta = ReadMetaFromFile(file);
				Register(meta);
			}
			else
			{
				AssetFileMetadata meta = WriteNewMetaForFile(file);
				Register(meta);
			}
		}
	}

	AssetFileMetadata AssetRegistry::WriteNewMetaForFile(const std::filesystem::path& file) const
	{
		const std::filesystem::path metaPath = file.string() + m_metaFileExtention;

		IAssetImporter* importer = m_manager->GetImporterForExtention(file.extension().string());

		AssetFileMetadata meta;
		if(importer)
		{
			importer->CreateMeta(file, meta);
		}
		else
		{
			// Fall back on default meta creation if no importer is found with the given file type.
			meta.Path = file;
			meta.SourceFileID = UUID::Generate();

			BF_CORE_LOG_WARN("Unable to create meta file for asset: %s, no importer found for filetype: %s", file.string().c_str(), file.extension().string().c_str());
		}



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

	AssetFileMetadata AssetRegistry::ReadMetaFromFile(const std::filesystem::path& file) const
	{
		const std::filesystem::path metaPath = file.string() + m_metaFileExtention;

		YAML::Node node;
		node = YAML::LoadFile(metaPath.string());
		return node["Meta"].as<AssetFileMetadata>();
	}
}