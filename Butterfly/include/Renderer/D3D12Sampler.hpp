#pragma once
#include "D3D12/D3D12Common.hpp"
#include "Resource.hpp"

namespace Butterfly
{
	class BFSampler : public BFResource, private NonCopyable
	{
	public:
		BFSampler(const D3D12_SAMPLER_DESC* desc);
		~BFSampler();

		uint32_t View() const {return m_viewIndex; }

	private:
		uint32_t m_viewIndex;
	};

	class BFSamplerDesc
	{
	public:
		D3D12_SAMPLER_DESC Desc = {};
		BFSamplerDesc()
		{
			Desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			Desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			Desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			Desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			Desc.MipLODBias = 0.0f;
			Desc.MaxAnisotropy = 1;
			Desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
			Desc.BorderColor[0] = 0.0f;
			Desc.BorderColor[1] = 0.0f;
			Desc.BorderColor[2] = 0.0f;
			Desc.BorderColor[3] = 0.0f;
			Desc.MinLOD = 0.0f;
			Desc.MaxLOD = D3D12_FLOAT32_MAX;
		}


		static BFSamplerDesc LinearClamp()
		{
			BFSamplerDesc desc;
			desc.Desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			desc.Desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			desc.Desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			desc.Desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			return desc;
		}

		static BFSamplerDesc LinearWrap()
		{
			BFSamplerDesc desc;
			desc.Desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			desc.Desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			desc.Desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			desc.Desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			return desc;
		}

		static BFSamplerDesc LinearBorder()
		{
			BFSamplerDesc desc;
			desc.Desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			desc.Desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			desc.Desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			desc.Desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			desc.Desc.BorderColor[0] = 1.0f;
			desc.Desc.BorderColor[1] = 1.0f;
			desc.Desc.BorderColor[2] = 1.0f;
			desc.Desc.BorderColor[3] = 1.0f;
			return desc;
		}
	};

	class BFSamplerCache : public NonCopyableNonMoveable
	{
	public:
		static BFSampler& GetOrCreate(const BFSamplerDesc& desc);

	private:
		inline static std::unordered_map<uint64_t, RefPtr<BFSampler>> s_samplerCache;
	};
}
