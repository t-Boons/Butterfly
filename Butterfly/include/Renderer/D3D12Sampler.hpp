#pragma once
#include "D3D12/D3D12Common.hpp"
#include "Resource.hpp"

namespace Butterfly
{
	class Sampler : public Resource, private NonCopyable
	{
	public:
		Sampler(const D3D12_SAMPLER_DESC* desc);
		~Sampler();

		uint32_t GetViewIndex() const {return m_viewIndex; }

	private:
		uint32_t m_viewIndex;
	};

	class SamplerDesc
	{
	public:
		D3D12_SAMPLER_DESC Desc = {};
		SamplerDesc()
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


		static SamplerDesc LinearClamp()
		{
			SamplerDesc desc;
			desc.Desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			desc.Desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			desc.Desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			desc.Desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			return desc;
		}

		static SamplerDesc LinearWrap()
		{
			SamplerDesc desc;
			desc.Desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			desc.Desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			desc.Desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			desc.Desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			return desc;
		}

		static SamplerDesc LinearBorder()
		{
			SamplerDesc desc;
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

		static SamplerDesc PointClamp()
		{
			SamplerDesc desc;
			desc.Desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			desc.Desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			desc.Desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			desc.Desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			return desc;
		}
	};

	class SamplerCache : public NonCopyableNonMoveable
	{
	public:
		static Sampler& GetOrCreate(const SamplerDesc& desc);

	private:
		inline static std::unordered_map<uint64_t, RefPtr<Sampler>> s_samplerCache;
	};
}
