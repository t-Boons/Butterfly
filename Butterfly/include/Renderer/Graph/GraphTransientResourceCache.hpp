#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{

#define GRAPHRESOURCE_LIFETIME 5

	class GraphTransientResourceCache : private NonCopyable
	{
	public:
		~GraphTransientResourceCache();

		void UpdateLifetimes();
		void Flush();

		bool HasValue(const std::string& key) const;

		uint32_t NumResources() const { return static_cast<uint32_t>(m_resources.size()); }

		template<typename Type, typename Args>
		Type& GetOrCreateTexture(const std::string& key, Args&& args);

	private:
		template<typename Type, typename Args>
		Type* AllocTexture(const std::string& key, Args&& args);

		void DeallocDeadResources();

		std::unordered_map<std::string, std::any> m_resources;
		std::unordered_map<std::string, uint32_t> m_lifetimes;
	};


	// Implementation.

	inline GraphTransientResourceCache::~GraphTransientResourceCache()
	{
		BF_PROFILE_EVENT();

		for (auto& resource : m_resources)
		{
			resource.second.reset();
		}
	}

	inline void GraphTransientResourceCache::Flush()
	{
		BF_PROFILE_EVENT();

		// Destroy All the resources.
		for (auto& resource : m_resources)
		{
			resource.second.reset();
		}

		m_lifetimes.clear();
		m_resources.clear();

		Log::Info("Deallocated/Flushed all transient resources.");
	}

	inline void GraphTransientResourceCache::UpdateLifetimes()
	{
		BF_PROFILE_EVENT();

		for (auto& lifetime : m_lifetimes)
		{
			lifetime.second--;
		}

		DeallocDeadResources();
	}

	inline bool GraphTransientResourceCache::HasValue(const std::string& key) const
	{
		return m_lifetimes.find(key) != m_lifetimes.end();
	}

	inline void GraphTransientResourceCache::DeallocDeadResources()
	{
		BF_PROFILE_EVENT();

		// Caching the resources to delete to not break the for loop iterator.
		std::queue<std::string> toDealloc;

		for (auto& lifetime : m_lifetimes)
		{
			const std::string& key = lifetime.first;

			// Resource lifetime has ended. Deallocate it.
			if (lifetime.second == 0)
			{
				toDealloc.push(key);
			}
		}

		// Here we make sure to delete everything in the toDealloc queue.
		while (!toDealloc.empty())
		{
			const std::string key = toDealloc.front();
			toDealloc.pop();

			std::any& resource = m_resources.at(key);
			resource.reset();

			m_lifetimes.erase(key);
			m_resources.erase(key);

			Log::Info("Deallocated Transient resource: %s", key.c_str());
		}
	}

	template<typename Type, typename Args>
	inline Type& GraphTransientResourceCache::GetOrCreateTexture(const std::string& key, Args&& args)
	{
		BF_PROFILE_EVENT();

		auto it = m_resources.find(key);
		if (it == m_resources.end())
		{
			Type* newResource = AllocTexture<Type, Args>(key, args);
			return *newResource;
		}

		// Get an existing resource.
		RefPtr<Type> out = std::any_cast<RefPtr<Type>>(it->second);

		// Reset the resource's lifetime.
		m_lifetimes[key] = GRAPHRESOURCE_LIFETIME;

		Check(out)
		return *out.get();
	}

	template<typename Type, typename Args>
	inline Type* GraphTransientResourceCache::AllocTexture(const std::string& key, Args&& args)
	{
		std::stringstream ss;
		ss << "Butterfly::GraphTransientResourceCache::AllocTexture -> " << key;
		BF_PROFILE_EVENT_DYNAMIC(ss.str().c_str());

		// Initialize a new resource.
		RefPtr<Type> newResource = RefPtr<Type>(new Type(std::forward<Args>(args)));
		m_resources[key] = newResource;
		m_lifetimes[key] = GRAPHRESOURCE_LIFETIME;

		Log::Info("Allocated new Transient resource: %s", key.c_str());
		return newResource.get();
	}
}