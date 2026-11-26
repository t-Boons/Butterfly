#include "Renderer/D3D12/D3D12ShaderVariables.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12View.hpp"

namespace Butterfly
{
    ShaderVariableBuilder &ShaderVariableBuilder::Add(const View &view)
    {
        m_list.PrepareForUse(view);
        return Add(view.GetViewIndex());
    }

    ShaderVariableBuilder &ShaderVariableBuilder::Add(const Sampler &sampler)
    {
        return Add(sampler.GetViewIndex());
    }

    ShaderVariableBuilder &ShaderVariableBuilder::Add(int index)
    {
        m_data.push_back(index);
        return *this;
    }

    void ShaderVariableBuilder::Submit()
    {
        m_list.SetRootConstants(m_data.data(), static_cast<uint32_t>(m_data.size()));
    }
}
