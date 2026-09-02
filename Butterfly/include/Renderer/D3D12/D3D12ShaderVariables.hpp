#pragma once
#include "D3D12Common.hpp"

namespace Butterfly
{
	class D3D12CommandList;

	class ShaderVariables
	{
	public:
		ShaderVariables();

		ShaderVariables& Add(int index);
		ShaderVariables& Submit(D3D12CommandList& list);
		ShaderVariables& Reset();

	private:
		uint32_t numDwords = 0;
		std::vector<int> m_bufferIndices;
	};
}
