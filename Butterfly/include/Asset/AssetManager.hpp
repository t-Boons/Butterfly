#pragma once
#include "Core/Common.hpp"
#include "Asset.hpp"
#include "AssetRegistry.hpp"
#include "Asset/Importer/AssetImporter.hpp"
#include "Asset/AssetUUID.hpp"

namespace Butterfly
{
    template<typename T>
    struct AssetHandle;

    class AssetManager : public NonCopyableNonMoveable
    {
    public:
        AssetManager();

        void Tick();

        AssetRegistry& GetAssetRegistry() { return m_assetRegistry; }

        template<typename T>
        T* Resolve(const AssetHandle<T>& handle) const;

        template<typename T>
        bool Acquire(const AssetUUID<T>& id, AssetHandle<T>& ret);

        bool IsType(const std::type_info& type, const UUID& id) const;

        const std::vector<RefPtr<IAssetImporter>>& GetImporters() const { return s_importers; }

        void GarbageCollect();

		template<typename T>
        AssetHandle<T> AddAssetEntry(const AssetEntry& entry);

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
        uint32_t  m_tickCounter = 0;
    };



    template<typename T>
    T* AssetManager::Resolve(const AssetHandle<T>& handle) const
    {
        auto it = m_entries.find(handle.GetID());
        if (it == m_entries.end()) return nullptr;
        return static_cast<T*>(it->second.Data.get());
    }

    template<typename T>
    bool AssetManager::Acquire(const AssetUUID<T>& id, AssetHandle<T>& ret)
    {
        if (!id.ID().Valid())
        {
            BF_CORE_LOG_ERROR("AssetManager::Acquire: Invalid UUID");
            return false;
        }

		auto it = m_entries.find(id.ID());
        if (it != m_entries.end())
        {
            ret = AssetHandle<T>(this, id.ID());
            return true;
        }

        AssetMetadata meta;

        if (!m_assetRegistry.Find(id.ID(), meta))
        {
            BF_CORE_LOG_ERROR("Meta for ID: %s cannot be found", id.ToString().c_str());
            return false;
        }

        IAssetImporter* importer = nullptr;
        for (auto& im : s_importers)
        {
            if (im->CanImport(meta.Extention) && im->CanImportType(typeid(T)))
            {
                importer = im.get();
                break;
            }
        }

        if (!importer)
        {
            BF_CORE_LOG_ERROR("No Importer found for filetype: %s and type: %s", meta.Extention.c_str(), typeid(T).name());
            return false;
        }

        if (!importer->Import(meta, *this))
        {
            BF_CORE_LOG_ERROR("Import for ID failed: %s", meta.ID.ToString().c_str());
            return false;
        }

        ret = AssetHandle<T>(this, id.ID());
        return true;
    }

    template<typename T>
    AssetHandle<T> AssetManager::AddAssetEntry(const AssetEntry& entry)
    {
		auto it = m_entries.find(entry.ID);
        if(it == m_entries.end())
        {
            m_entries[entry.ID] = entry;
            return AssetHandle<T>(this, entry.ID);
        }

        BF_CORE_LOG_WARN("Asset with ID: %s already exists", entry.ID.ToString().c_str());
        return AssetHandle<T>(this, entry.ID);
    }
}