#pragma once
#include "Core/Common.hpp"
#include "Asset.hpp"
#include "AssetRegistry.hpp"
#include "Asset/Importer/AssetImporter.hpp"

namespace Butterfly
{
    template<typename T>
    struct AssetHandle;

	class AssetManager : public NonCopyableNonMoveable
	{
    public:
        AssetManager();

        AssetRegistry& GetAssetRegistry() { return m_assetRegistry; }

        template<typename T>
        T* Resolve(const AssetHandle<T>& handle) const;

        template<typename T>
        AssetHandle<T> Acquire(const UUID& id);

        const std::vector<RefPtr<IAssetImporter>>& GetImporters() const { return s_importers; }


    private:
        template<typename T>
        friend class AssetHandle;


        template<typename T>
        friend class ImporterRegistrar;

        template<typename T>
        static void RegisterImporter()
        {
            BF_CORE_LOG_INFO("Registering Asset Importer: %s", typeid(T).name());
            s_importers.push_back(MakeRef<T>());
        }

        void AddRef(const UUID& id);
        void SubtractRef(const UUID& id);

        inline static std::vector<RefPtr<IAssetImporter>> s_importers;
        AssetRegistry m_assetRegistry;
        std::unordered_map<UUID, AssetEntry> m_entries;
	};



    template<typename T>
    T* AssetManager::Resolve(const AssetHandle<T>& handle) const
    {
        auto it = m_entries.find(handle.GetID());
        if (it == m_entries.end()) return nullptr;
        return static_cast<T*>(it->second.Data.get());
    }

    template<typename T>
    AssetHandle<T> AssetManager::Acquire(const UUID& id)
    {
        auto& entry = m_entries[id];

        AssetMetadata meta;

        if (!m_assetRegistry.Find(id, meta))
        {
            BF_CORE_LOG_ERROR("Meta for ID: %s cannot be found", id.ToString());
            return {};
        }

        IAssetImporter* importer = nullptr;
        for (auto& im : s_importers)
        {
            if (im->CanImport(meta.Extention))
            {
                importer = im.get();
                break;
            }
        }

        if (!importer)
        {
            BF_CORE_LOG_ERROR("No Importer found for filetype: %s", meta.Extention.c_str());
            return {};
        }

        ImportResult result;
        if (!importer->Import(meta, result))
        {
            BF_CORE_LOG_ERROR("Import for ID failed: %s", meta.ID.ToString().c_str());
            return {};
        }

        entry.RefCount++;

        auto& e = m_entries[id];
        e.ID = id;
        e.Data = result.Asset.Data;
        e.Type = result.Asset.Type;

        return AssetHandle<T>(this, id);
    }
}