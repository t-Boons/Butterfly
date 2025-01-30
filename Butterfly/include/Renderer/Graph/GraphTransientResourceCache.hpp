#pragma once
#include "Core/Common.hpp"

namespace Butterfly
{

#define GRAPHRESOURCE_LIFETIME 5

	class BFRGResource;
	class BFRGTexture;
	struct BFRGTextureInitializer;

	class GraphTransientResourceCache : private NonCopyable
	{
	public:
		~GraphTransientResourceCache();

		void UpdateLifetimes();
		void Flush();

		bool HasValue(const std::string& key) const;

		uint32_t NumResources() const { return static_cast<uint32_t>(m_resources.size()); }

		BFRGTexture& GetOrCreateTexture(const BFRGTextureInitializer& initializer);

	private:

		BFRGTexture* AllocTexture(const BFRGTextureInitializer& initializer);

		void DeallocDeadResources();

		std::unordered_map<std::string, BFRGResource*> m_resources;
		std::unordered_map<std::string, uint32_t> m_lifetimes;
		
	};
}