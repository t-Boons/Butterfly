#pragma once
#include "D3D12Common.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12View.hpp"
#include "Renderer/D3D12Sampler.hpp"

namespace Butterfly
{
	class D3D12CommandList;

	class BFShaderVariableBuilder
	{
	public:
		BFShaderVariableBuilder(D3D12CommandList& list)
		: m_list(list) {}

		BFShaderVariableBuilder& Add(const BFView& view);
		BFShaderVariableBuilder& Add(const BFSampler& sampler);
		BFShaderVariableBuilder& Add(int index);
		void Submit();

	private:
		D3D12CommandList& m_list;
		std::vector<int> m_data;
	};
}
