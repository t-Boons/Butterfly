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

		BF_CORE_LOG_TRACE("Asset root path: %s", m_assetPath.string().c_str());

		Scan();
	}

	void AssetRegistry::Register(const AssetFileMetadata& meta)
	{
		BF_CORE_LOG_TRACE("Registering asset file: %ls", meta.Path.filename().c_str());
		m_registeredFiles[meta.SourceFileID] = meta;
		for (auto& asset : meta.Assets)
		{
			m_registeredAssets[asset.first] = asset.second;
		}
	}

	void AssetRegistry::UnRegister(const AssetFileMetadata& meta)
	{
		BF_CORE_LOG_TRACE("Unregistering asset file: %ls", meta.Path.filename().c_str());
		for (auto& asset : meta.Assets)
		{
			m_registeredAssets.erase(asset.first);
		}
		m_registeredFiles.erase(meta.SourceFileID);
	}

	void AssetRegistry::RenameFileAndMeta(const UUID& id, const std::filesystem::path& newPath)
	{
		auto it = m_registeredFiles.find(id);
		if (it == m_registeredFiles.end())
		{
			BF_CORE_LOG_ERROR("Unable to rename file, no file with ID: %s found in registry", id.ToString().c_str());
			return;
		}

		const std::filesystem::path oldFilePath = it->second.Path;
		const std::filesystem::path oldMetaPath = oldFilePath.string() + m_metaFileExtention;
		const std::filesystem::path newFilePath = newPath;
		const std::filesystem::path newMetaPath = newFilePath.string() + m_metaFileExtention;

		if (!FileSystem::Rename(oldFilePath, newFilePath))
		{
			BF_CORE_LOG_ERROR("Unable to rename file: %s to %s", oldFilePath.string().c_str(), newFilePath.string().c_str());
			return;
		}

		if(!FileSystem::Rename(oldMetaPath, newMetaPath))
		{
			BF_CORE_LOG_ERROR("Unable to rename meta file: %s to %s", oldMetaPath.string().c_str(), newMetaPath.string().c_str());
			return;
		}
		it->second.Path = newFilePath;
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

	bool AssetRegistry::DeleteFileAndMeta(const UUID& id)
	{
		auto it = m_registeredFiles.find(id);
		if (it == m_registeredFiles.end())
		{
			BF_CORE_LOG_ERROR("Unable to delete file, no file with ID: %s found in registry", id.ToString().c_str());
			return false;
		}
		const std::filesystem::path filePath = it->second.Path;
		const std::filesystem::path metaPath = filePath.string() + m_metaFileExtention;
		if (!FileSystem::Delete(filePath))
		{
			BF_CORE_LOG_ERROR("Unable to delete file: %s", filePath.string().c_str());
			return false;
		}
		if (!FileSystem::Delete(metaPath))
		{
			BF_CORE_LOG_ERROR("Unable to delete meta file: %s", metaPath.string().c_str());
			return false;
		}

		UnRegister(it->second);


		return true;
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
		const auto files = FileSystem::WalkDirectoryRecursive(m_assetPath);
		BF_CORE_LOG_INFO("AssetRegistry::Scan: NumFiles: %d, Directory: %s", files.size(), m_assetPath.string().c_str());
		for (auto& file : files)
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

		try
		{
			YAML::Node node;
			node = YAML::LoadFile(metaPath.string());
			AssetFileMetadata meta = node["Meta"].as<AssetFileMetadata>();
			meta.Path = file;
			return meta;
		}
		catch (const YAML::Exception& e)
		{
			BF_CORE_LOG_ERROR("Failed to read meta file: %s, Error: %s", metaPath.string().c_str(), e.what());
			return {};
		}
	}
}