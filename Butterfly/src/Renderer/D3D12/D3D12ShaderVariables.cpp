#include "Renderer/D3D12/D3D12ShaderVariables.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"

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

    ShaderVariables& ShaderVariables::Submit(D3D12CommandList& list, bool compute)
    {
        BF_PROFILE_EVENT();

        if (compute)
        {
            list.List()->SetComputeRoot32BitConstants(
                0u,
                numDwords,
                m_bufferIndices.data(),
                0u
            );
        }
        else
        {
            list.List()->SetGraphicsRoot32BitConstants(
                0u,
                numDwords,
                m_bufferIndices.data(),
                0u
            );
        }

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
