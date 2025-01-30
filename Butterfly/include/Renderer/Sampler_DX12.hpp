#pragma once
#include "D3D12/Common_DX12.hpp"
#include "Resource.hpp"

namespace Butterfly
{
	class BFSampler : public BFResource, private NonCopyable
	{
	public:
		BFSampler(const D3D12_SAMPLER_DESC* desc = nullptr);
		~BFSampler();

		uint32_t View() const {return m_viewIndex; }

	private:
		uint32_t m_viewIndex;
	};
}