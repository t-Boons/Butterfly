#pragma once
#include "D3D12Common.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"

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

	struct ShaderVariablesFromStruct
	{
		public:
			template <typename T>
			static void Submit(const T& value, D3D12CommandList& list)
			{
				BF_PROFILE_EVENT();
				if (list.CurrentPassType() == D3D12CommandList::PassType::Compute)
				{
					list.List()->SetComputeRoot32BitConstants(
						0u,
						sizeof(T) / 4,
						&value,
						0u
					);
				}
				else
				{
					list.List()->SetGraphicsRoot32BitConstants(
						0u,
						sizeof(T) / 4,
						&value,
						0u
					);
				}
			}
	};
}
