#pragma once
#include "D3D12Common.hpp"

namespace Butterfly
{
	struct D3D12SubresourceRange
	{
		uint32_t FirstMip   = 0;
		uint32_t MipCount   = BF_ALL;
		uint32_t FirstSlice = 0;
		uint32_t SliceCount = BF_ALL;

		bool IsAll() const
		{
			return MipCount == BF_ALL && SliceCount == BF_ALL && FirstMip == 0 && FirstSlice == 0;
		}

		D3D12SubresourceRange& MipRange(uint32_t firstMip, uint32_t mipCount)
		{
			FirstMip = firstMip;
			MipCount = mipCount;
			return *this;
		}
		
		D3D12SubresourceRange& SliceRange(uint32_t firstSlice, uint32_t sliceCount)
		{
			FirstSlice = firstSlice;
			SliceCount = sliceCount;
			return *this;
		}

		D3D12SubresourceRange& FullRange()
		{
			FirstMip = 0;
			MipCount = BF_ALL;
			FirstSlice = 0;
			SliceCount = BF_ALL;
			return *this;
		}
	};
}