#pragma once
#include "Core/Common.hpp"
#include "Asset.hpp"
#include "AssetRegistry.hpp"
#include "Asset/Importer/AssetImporter.hpp"

namespace Butterfly
{
    template<typename T>
    class AssetHandle
    {
    public:
        UUID ID;
    };

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

            const AssetMetadata* meta = m_assetRegistry.Find(id);
            if (!meta)
            {
                BF_CORE_LOG_ERROR("Meta for ID: %s cannot be found", id.ToString());
                return {};
            }

            IAssetImporter* importer = nullptr;
            for (auto& im : m_importers)
            {
                if (im->CanImport(meta->Extention))
                {
                    importer = im;
                    break;
                }
            }

            if (!importer)
            {
                BF_CORE_LOG_ERROR("No Importer found for filetype: %s", meta->Extention);
                return {};
            }

            ImportResult result = importer->Import(meta);

            entry.RefCount++;

            for (auto& asset : result.Assets)
            {
                auto& e = m_entries[asset.ID];
                e.ID = asset.ID;
                e.Data = asset.Data;
                e.Type = asset.Type;
            }

            return AssetHandle<T>(id);
        }

        const std::vector<RefPtr<IAssetImporter>>& GetImporters() const { return m_importers; }

    private:
        AssetRegistry m_assetRegistry;
        std::vector<RefPtr<IAssetImporter>> m_importers;
        std::unordered_map<UUID, AssetEntry> m_entries;
	};
}