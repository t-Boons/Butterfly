#pragma once
#include "Core/Common.hpp"
#include <any>
#include <unordered_map>
#include <string>

namespace Butterfly
{
    class Blackboard : private NonCopyable
    {
    public:
        template <typename ResourceType>
        void Register(RefPtr<ResourceType> value, const std::string& key);

        template <typename ResourceType>
        RefPtr<ResourceType> Get(const std::string& key) const;

        template <typename ResourceType>
        bool TryGet(const std::string& key, RefPtr<ResourceType>& out) const;

        template <typename ResourceType>
        void Replace(RefPtr<ResourceType> value, const std::string& key);

        template <typename ResourceType>
        void RegisterOrReplace(RefPtr<ResourceType> value, const std::string& key);


        void Erase(const std::string& key);

        bool HasValue(const std::string& name) const;

        uint32_t NumResources() const { return static_cast<uint32_t>(m_data.size()); }

    private:
        std::unordered_map<std::string, std::any> m_data;
    };

    ////////////////////
    // Implementation //
    ////////////////////

    template <typename ResourceType>
    inline void Blackboard::Register(RefPtr<ResourceType> value, const std::string& key)
    {
        BF_PROFILE_EVENT();

        BF_CORE_ASSERT(!HasValue(key), "Duplicate key found: %s", key.c_str());
        m_data[key] = std::move(value);
    }

    template <typename ResourceType>
    inline RefPtr<ResourceType> Blackboard::Get(const std::string& key) const
    {
        BF_PROFILE_EVENT();

        auto it = m_data.find(key);
        if (it != m_data.end())
        {
#ifdef BUTTERFLY_DEBUG
            try
            {
                return std::any_cast<RefPtr<ResourceType>>(it->second);
            }
            catch (const std::bad_any_cast& e)
            {
                BF_CORE_LOG_ERROR(e.what());
                BF_CORE_ASSERT(false, "Blackboard key value \"%s\" is not of type %s", key.c_str(), typeid(ResourceType).name());
            }
#else
            return std::any_cast<RefPtr<ResourceType>>(it->second);
#endif
        }

        BF_CORE_ASSERT(false, "Blackboard does not have value with key: %s", key.c_str());
        return nullptr;
    }

    template <typename ResourceType>
    inline bool Blackboard::TryGet(const std::string& key, RefPtr<ResourceType>& out) const
    {
        if (!HasValue(key))
        {
            return false;
        }

        out = Get<ResourceType>(key);
        return true;
    }


    inline void Blackboard::Erase(const std::string& key)
    {
        BF_PROFILE_EVENT();

        BF_CORE_ASSERT(HasValue(key), "Blackboard does not have value with key: %s", key.c_str());
        m_data.erase(m_data.find(key));
    }

    template <typename ResourceType>
    inline void Blackboard::Replace(RefPtr<ResourceType> value, const std::string& key)
    {
        BF_PROFILE_EVENT();

        BF_CORE_ASSERT(HasValue(key), "Blackboard does not have value with key: %s", key.c_str());
        m_data[key] = std::move(value);
    }

    template <typename ResourceType>
    inline void Blackboard::RegisterOrReplace(RefPtr<ResourceType> value, const std::string& key)
    {
        if (HasValue(key))
        {
            Replace<ResourceType>(std::move(value), key);
        }
        else
        {
            Register<ResourceType>(std::move(value), key);
        }
    }

    inline bool Blackboard::HasValue(const std::string& name) const
    {
        BF_PROFILE_EVENT();

        auto it = m_data.find(name);
        return it != m_data.end();
    }
}