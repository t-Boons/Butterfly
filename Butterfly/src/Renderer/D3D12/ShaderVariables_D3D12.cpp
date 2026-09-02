#include "Renderer/D3D12/ShaderVariables_DX12.hpp"
#include "Renderer/D3D12/CommandList_DX12.hpp"

namespace Butterfly
{
	ShaderVariables::ShaderVariables()
	{
		BF_PROFILE_EVENT();

		m_bufferIndices.reserve(8);
	}

	ShaderVariables& ShaderVariables::Add(int index)
	{
		BF_PROFILE_EVENT();

		m_bufferIndices.push_back(index);
		numDwords++;
		return *this;
	}

	ShaderVariables& ShaderVariables::Submit(DX12CommandList& list)
	{
		BF_PROFILE_EVENT();

		list.List()->SetGraphicsRoot32BitConstants(0u, numDwords, reinterpret_cast<const void*>(&m_bufferIndices[0]), 0u);
		return *this;
	}

	ShaderVariables& ShaderVariables::Reset()
	{
		BF_PROFILE_EVENT();

		numDwords = 0;
		m_bufferIndices.clear();
		return *this;
	}
}