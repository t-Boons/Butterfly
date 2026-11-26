#include "Renderer/D3D12Sampler.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12DescriptorAllocator.hpp"

namespace Butterfly
{
	Sampler::Sampler(const D3D12_SAMPLER_DESC* desc)
	{
        BF_PROFILE_EVENT();

        const D3D12_SAMPLER_DESC* samplerDesc = desc;
		m_viewIndex = D3D12API()->DescriptorAllocatorSampler()->CreateSampler(samplerDesc);
	}

	Sampler::~Sampler()
	{
		D3D12API()->DescriptorAllocatorSampler()->FreeHandle(m_viewIndex);
	}

    Sampler& SamplerCache::GetOrCreate(const SamplerDesc& desc)
    {
		BF_PROFILE_EVENT();

		uint64_t hash = std::hash<std::string>()(std::string(reinterpret_cast<const char*>(&desc), sizeof(SamplerDesc)));
		auto it = s_samplerCache.find(hash);
		if (it != s_samplerCache.end())
		{
			return *(it->second);
		}
		else
		{
			RefPtr<Sampler> newSampler = MakeRef<Sampler>(&desc.Desc);
			s_samplerCache[hash] = newSampler;
			return *newSampler;
		}
    }
}
