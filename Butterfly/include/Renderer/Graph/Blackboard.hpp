#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{
	class Blackboard : private NonCopyable
	{
	public:
        template <typename ResourceType>
        void Register(ResourceType* value, const std::string& key);

        template <typename ResourceType>
        ResourceType& Get(const std::string& key) const;

        template <typename ResourceType>
        void Replace(ResourceType* value, const std::string& key);

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
    inline void Blackboard::Register(ResourceType* value, const std::string& key)
    {
        BF_PROFILE_EVENT();

        CheckMsg(!HasValue(key), "Duplicate key found: %s", key.c_str());
        m_data[key] = value;
    }

    template <typename ResourceType>
    inline ResourceType& Blackboard::Get(const std::string& key) const
    {
        BF_PROFILE_EVENT();

        auto it = m_data.find(key);
        if (it != m_data.end())
        {
            ResourceType* ptr = nullptr;
#ifdef BUTTERFLY_DEBUG
            try
            {
                ptr = std::any_cast<ResourceType*>(it->second);
                CheckMsg(ptr, "Resource with name %s does not exist in Blackboard.", key.c_str())
                return *ptr;
            }
            catch (const std::exception& e)
            {
                Log::Error(e.what());
                Log::Assert(false, "Blackboard key value \"%s\" is not of type %s", key.c_str(), typeid(ResourceType).name());
            }
#else
            ptr = std::any_cast<ResourceType*>(it->second);
            return *ptr;
#endif
        }

        Throw("Blackboard does not have value with key: %s", key.c_str());
        ResourceType* errorDummy = nullptr;
        return *errorDummy;
    }


    inline void Blackboard::Erase(const std::string& key)
    {
        BF_PROFILE_EVENT();

        CheckMsg(HasValue(key), "Blackboard does not have value with key: %s", key.c_str());
        m_data.erase(m_data.find(key));
    }

    template <typename ResourceType>
    inline void Blackboard::Replace(ResourceType* value, const std::string& key)
    {
        BF_PROFILE_EVENT();

        CheckMsg(HasValue(key), "Blackboard does not have value with key: %s", key.c_str());
        m_data[key] = value;
    }

    inline bool Blackboard::HasValue(const std::string& name) const
    {
        BF_PROFILE_EVENT();

        auto it = m_data.find(name);
        if (it != m_data.end())
        {
            return true;
        }
        return false;
    }
}