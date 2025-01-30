#pragma once
#include "Common_DX12.hpp"

namespace Butterfly
{
	class DX12CommandList;

	class ShaderVariables
	{
	public:
		ShaderVariables();

		ShaderVariables& Add(int index);
		ShaderVariables& Submit(DX12CommandList& list);
		ShaderVariables& Reset();

	private:
		uint32_t numDwords = 0;
		std::vector<int> m_bufferIndices;
	};
}