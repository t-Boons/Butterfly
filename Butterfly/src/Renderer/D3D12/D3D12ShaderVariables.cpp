#include "Renderer/D3D12/D3D12ShaderVariables.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12View.hpp"

namespace Butterfly
{
    BFShaderVariableBuilder &BFShaderVariableBuilder::Add(const BFView &view)
    {
        m_list.PrepareForUse(view);
        return Add(view.View());
    }

    BFShaderVariableBuilder &BFShaderVariableBuilder::Add(const BFSampler &sampler)
    {
        return Add(sampler.View());
    }

    BFShaderVariableBuilder &BFShaderVariableBuilder::Add(int index)
    {
        m_data.push_back(index);
        return *this;
    }

    void BFShaderVariableBuilder::Submit()
    {
        m_list.SetRootConstants(m_data.data(), static_cast<uint32_t>(m_data.size()));
    }
}
