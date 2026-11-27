#pragma once
#include "Core/Common.hpp"

#define GRAPHRESOURCE_LIFETIME 3

namespace Butterfly
{
	class GraphTransientResourceCache : private NonCopyable
	{
	public:
		void UpdateLifetimes();
		void Flush();

		void FlushValue(const std::string& key);

		bool HasValue(const std::string& key) const;

		uint32_t NumResources() const { return static_cast<uint32_t>(m_resources.size()); }

		template<typename Type, typename Desc>
		Type& GetOrCreate(const std::string& key, const Desc& desc);

	private:
		template<typename Type, typename Args>
		Type* Alloc(const std::string& key, Args&& args);

		void ReleaseDeferred(std::any&& resource);

		struct Entry
		{
			std::any Resource;
			std::any Desc;
			uint32_t Lifetime = GRAPHRESOURCE_LIFETIME;
		};

		std::unordered_map<std::string, Entry> m_resources;
		std::vector<std::pair<std::any, uint32_t>> m_pendingRelease;
	};

	template<typename Type, typename Desc>
	inline Type& GraphTransientResourceCache::GetOrCreate(const std::string& key, const Desc& desc)
	{
		BF_PROFILE_EVENT();

		static_assert(std::equality_comparable<Desc>, "GraphTransientResourceCache: Desc must define bool operator==");

		Entry& entry = m_resources[key];

		const Desc* cachedDesc = std::any_cast<Desc>(&entry.Desc);
		const bool  needsCreate = !entry.Resource.has_value() || !cachedDesc || !(*cachedDesc == desc);

		if (needsCreate)
		{
			if (entry.Resource.has_value())
			{
				ReleaseDeferred(std::move(entry.Resource));
				BF_CORE_LOG_TRACE("Recreating transient resource (desc changed): %s", key.c_str());
			}

			entry.Resource = MakeRef<Type>(desc);
			entry.Desc     = desc;
			BF_CORE_LOG_TRACE("Allocated new transient resource: %s", key.c_str());
		}

		entry.Lifetime = GRAPHRESOURCE_LIFETIME;
		return *std::any_cast<RefPtr<Type>&>(entry.Resource);
	}

	inline void GraphTransientResourceCache::ReleaseDeferred(std::any&& resource)
	{
		m_pendingRelease.emplace_back(std::move(resource), NUM_RENDER_BUFFERS);
	}

	inline void GraphTransientResourceCache::UpdateLifetimes()
	{
		BF_PROFILE_EVENT();

		// Unused resources: move to the deferred queue once their lifetime ends
		for (auto it = m_resources.begin(); it != m_resources.end(); )
		{
			if (--it->second.Lifetime == 0)
			{
				ReleaseDeferred(std::move(it->second.Resource));
				it = m_resources.erase(it);
			}
			else
			{
				++it;
			}
		}

		// Deferred: actually free after the GPU is done with them
		for (auto it = m_pendingRelease.begin(); it != m_pendingRelease.end(); )
		{
			if (--it->second == 0)
				it = m_pendingRelease.erase(it);   // RefPtr inside the any is released here
			else
				++it;
		}
	}

	inline void GraphTransientResourceCache::Flush()
	{
		for (auto& [key, entry] : m_resources)
			ReleaseDeferred(std::move(entry.Resource));
		m_resources.clear();
	}
}