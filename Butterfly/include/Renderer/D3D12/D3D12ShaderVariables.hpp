#pragma once
#include "D3D12Common.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12View.hpp"
#include "Renderer/D3D12Sampler.hpp"

namespace Butterfly
{
	class D3D12CommandList;

	class ShaderVariableBuilder
	{
	public:
		ShaderVariableBuilder(D3D12CommandList& list)
		: m_list(list) {}

		ShaderVariableBuilder& Add(const View& view);
		ShaderVariableBuilder& Add(const Sampler& sampler);
		ShaderVariableBuilder& Add(int index);
		void Submit();

	private:
		D3D12CommandList& m_list;
		std::vector<int> m_data;
	};
}
