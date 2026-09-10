#pragma once
#include "Core/Common.hpp"
#include "Asset.hpp"
#include "AssetRegistry.hpp"
#include "Asset/Importer/AssetImporter.hpp"

namespace Butterfly
{

	class AssetManager : public NonCopyableNonMoveable
	{
    public:
        AssetManager();

        AssetRegistry& GetAssetRegistry() { return m_assetRegistry; }

        template<typename T>
        T* Resolve(const AssetHandle<T>& handle)
        {
            auto it = m_entries.find(handle.ID);
            if (it == m_entries.end()) return nullptr;
            return static_cast<T*>(it->second.Data.get());
        }

        template<typename T>
        AssetHandle<T> Acquire(UUID id)
        {
            auto& entry = m_entries[id];

            AssetMetadata meta;
             
            if (!m_assetRegistry.Find(id, meta))
            {
                BF_CORE_LOG_ERROR("Meta for ID: %s cannot be found", id.ToString());
                return {};
            }

            IAssetImporter* importer = nullptr;
            for (auto& im : m_importers)
            {
                if (im->CanImport(meta.Extention))
                {
                    importer = im.get();
                    break;
                }
            }

            if (!importer)
            {
                BF_CORE_LOG_ERROR("No Importer found for filetype: %s", meta.Extention);
                return {};
            }

			ImportResult result;
			if (!importer->Import(meta, result))
			{
                BF_CORE_LOG_ERROR("Import for ID failed: %s", meta.ID.ToString());
                return {};
			}

            entry.RefCount++;

            auto& e = m_entries[result.Asset.ID];
            e.ID = result.Asset.ID;
            e.Data = result.Asset.Data;
            e.Type = result.Asset.Type;

            return AssetHandle<T>(id);
        }

        const std::vector<RefPtr<IAssetImporter>>& GetImporters() const { return m_importers; }

    private:
        AssetRegistry m_assetRegistry;
        std::vector<RefPtr<IAssetImporter>> m_importers;
        std::unordered_map<UUID, AssetEntry> m_entries;
	};
}