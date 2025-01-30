#include "Renderer/Graph/GraphTransientResourceCache.hpp"
#include "Renderer/Graph/GraphTypes.hpp"
#include "Renderer/Texture_DX12.hpp"
#include "Renderer/Buffer_DX12.hpp"

namespace Butterfly
{
	GraphTransientResourceCache::~GraphTransientResourceCache()
	{
		BF_PROFILE_EVENT();

		for (auto& resource : m_resources)
		{
			FREE(resource.second);
		}
	}


	BFRGTexture& GraphTransientResourceCache::GetOrCreateTexture(const BFRGTextureInitializer& initializer)
	{
		BF_PROFILE_EVENT();

		const std::string& key = initializer.Name;

		auto it = m_resources.find(key);
		if (it == m_resources.end())
		{
			auto* newResource = AllocTexture(initializer);
			return *newResource;
		}

		// Get an existing resource.
		auto* out = reinterpret_cast<BFRGTexture*>(it->second);

		// Reset the resource's lifetime.
		m_lifetimes[key] = GRAPHRESOURCE_LIFETIME;

		Check(out)
		return *out;
	}

	void GraphTransientResourceCache::Flush()
	{
		BF_PROFILE_EVENT();

		// Destroy All the resources.
		for (auto& resource : m_resources)
		{
			FREE(resource.second);
		}

		m_lifetimes.clear();
		m_resources.clear();

		Log::Info("Deallocated/Flushed all transient resources.");
	}

	void GraphTransientResourceCache::UpdateLifetimes()
	{
		BF_PROFILE_EVENT();


		// Caching the resources to delete to not break the for loop iterator.
		std::queue<std::string> toDelete;

		for (auto& lifetime : m_lifetimes)
		{
			lifetime.second--;
		}

		DeallocDeadResources();
	}

	bool GraphTransientResourceCache::HasValue(const std::string& key) const
	{
		return m_lifetimes.find(key) != m_lifetimes.end();
	}

	BFRGTexture* GraphTransientResourceCache::AllocTexture(const BFRGTextureInitializer& initializer)
	{
		std::stringstream ss;
		ss << "Butterfly::GraphTransientResourceCache::AllocTexture -> " << initializer.Name;
		BF_PROFILE_EVENT_DYNAMIC(ss.str().c_str());

		const std::string& key = initializer.Name;

		// Initialize a new resource.
		auto* newResource = new BFRGTexture(initializer);
		m_resources[key] = newResource;
		m_lifetimes[key] = GRAPHRESOURCE_LIFETIME;

		Log::Info("Allocated new Transient resource: %s", key.c_str());
		return newResource;
	}

	void GraphTransientResourceCache::DeallocDeadResources()
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

			BFRGResource* resource = m_resources.at(key);
			Check(resource);
			FREE(resource);

			m_lifetimes.erase(key);
			m_resources.erase(key);

			Log::Info("Deallocated Transient resource: %s", key.c_str());
		}
	}
}