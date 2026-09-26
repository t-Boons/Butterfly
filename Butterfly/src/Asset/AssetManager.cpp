#include "Asset/AssetManager.hpp"

namespace Butterfly
{
	AssetManager::AssetManager()
        : m_assetRegistry(this)
    {
    }

	void AssetManager::Tick()
	{
		if (m_tickCounter % 60 == 0)
		{
			GarbageCollect();
		}

		m_tickCounter++;
	}

	void AssetManager::GarbageCollect()
	{
		for (auto it = m_entries.begin(); it != m_entries.end();)
		{
			if (it->second.RefCount == 0 && !it->second.Persistant)
			{
				BF_CORE_LOG_TRACE("Garbage collecting asset: %s", it->first.ToString().c_str());
				it = m_entries.erase(it);
			}
			else
			{
				it++;
			}
		}
	}

	void AssetManager::AddRef(const UUID& id)
	{
		auto it = m_entries.find(id);
		BF_CORE_ASSERT(it != m_entries.end(), "Trying to add ref to non-existing asset");
		it->second.RefCount++;
	}

	void AssetManager::SubtractRef(const UUID& id)
	{
		auto it = m_entries.find(id);
		BF_CORE_ASSERT(it != m_entries.end(), "Trying to subtract ref from non-existing asset");
		BF_CORE_ASSERT(it->second.RefCount > 0, "Trying to subtract ref from asset with 0 refs");

		it->second.RefCount--;

		if (it->second.RefCount == 0 && !it->second.Persistant)
		{
			BF_CORE_LOG_TRACE("Asset: %s has 0 refs and is not persistant, will be garbage collected", id.ToString().c_str());
		}
	}

	IAssetImporter* AssetManager::GetImporterForExtention(const std::string& fileExtention) const
	{
		for (auto& im : s_importers)
		{
			if (im->CanImport(fileExtention))
			{
				return im.get();
			}
		}
		return nullptr;
	}

	bool AssetManager::AssetEntryExists(const UUID& id) const
	{
		return m_entries.find(id) != m_entries.end();
	}

	bool AssetManager::LoadSourceFile(const AssetMetadata& meta)
	{
		AssetFileMetadata fileMeta;
		m_assetRegistry.FindFile(meta.SourceFileID, fileMeta);

		IAssetImporter* importer = GetImporterForExtention(fileMeta.Path.extension().string());

		if (!importer)
		{
			BF_CORE_LOG_ERROR("No Importer found for filetype: %s", fileMeta.Path.extension().string().c_str());
			return false;
		}

		if (!importer->Import(fileMeta, *this))
		{
			BF_CORE_LOG_ERROR("Import for ID failed: %s", meta.SourceFileID.ToString().c_str());
			return false;
		}

		return true;
	}
}