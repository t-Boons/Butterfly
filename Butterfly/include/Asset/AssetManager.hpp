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

        const std::vector<RefPtr<IAssetImporter>>& GetImporters() const { return s_importers; }

        void GarbageCollect();

		template<typename T>
        AssetHandle<T> AddAssetEntry(const AssetEntry& entry);

		bool AssetEntryExists(const UUID& id) const;

        template<typename T>
        IAssetImporter* GetImporter(const std::filesystem::path& filePath) const;

        IAssetImporter* GetImporterForExtention(const std::string& fileExtention) const;

    private:
        bool LoadSourceFile(const AssetMetadata& meta);

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
    inline T* AssetManager::Resolve(const AssetHandle<T>& handle) const
    {
        auto it = m_entries.find(handle.GetID());
        if (it != m_entries.end())
        {
			return static_cast<T*>(it->second.Data.get());
        }
        return nullptr;
    }

    template<typename T>
    inline bool AssetManager::Acquire(const AssetUUID<T>& id, AssetHandle<T>& ret)
    {
        if (!id.ID().Valid())
        {
            BF_CORE_LOG_ERROR("AssetManager::Acquire: Invalid UUID");
            return false;
        }

		// Asset is already loaded, return the handle.
		auto it = m_entries.find(id.ID());
        if (it != m_entries.end())
        {
			BF_CORE_LOG_TRACE("Asset with ID: %s is already loaded, returning handle", id.ToString().c_str());
            ret = AssetHandle<T>(this, id.ID());
            return true;
        }

        AssetMetadata meta;

        if (!m_assetRegistry.FindAsset(id.ID(), meta))
        {
            BF_CORE_LOG_ERROR("Meta for ID: %s cannot be found", id.ToString().c_str());
            return false;
        }

        if(meta.Type != T::Type)
        {
            BF_CORE_LOG_ERROR("Meta for ID: %s is not of type: %s", id.ToString().c_str(), typeid(T).name());
            return false;
        }

		BF_CORE_LOG_INFO("Loading asset with ID: %s", id.ToString().c_str());
        LoadSourceFile(meta);
		BF_CORE_LOG_INFO("Asset loaded with ID: %s", id.ToString().c_str());

        BF_CORE_ASSERT(m_entries.find(id.ID()) != m_entries.end(), "Asset does not exist in the asset entries.");

        ret = AssetHandle<T>(this, id.ID());
        return true;
    }

    template<typename T>
    inline AssetHandle<T> AssetManager::AddAssetEntry(const AssetEntry& entry)
    {   
		auto it = m_entries.find(entry.ID);
        if(it == m_entries.end())
        {
            BF_CORE_LOG_TRACE("Adding asset entry of type %s with ID: %s", entry.Type.TypeName.c_str(), entry.ID.ToString().c_str());
            m_entries[entry.ID] = entry;
            return AssetHandle<T>(this, entry.ID);
        }

        BF_CORE_LOG_WARN("Asset with ID: %s already exists", entry.ID.ToString().c_str());
        return AssetHandle<T>(this, entry.ID);
    }

    template<typename T>
    inline IAssetImporter* AssetManager::GetImporter(const std::filesystem::path& filePath) const
    {
        for (auto& im : s_importers)
        {
            if (im->CanImport(filePath.extension().string()) && im->CanImportType(typeid(T)))
            {
                return im.get();
            }
        }
        return nullptr;
    }
}