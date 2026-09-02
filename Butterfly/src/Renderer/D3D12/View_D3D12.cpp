#include "Renderer/D3D12/View_DX12.hpp"
#include "Renderer/D3D12/DescriptorAllocator_DX12.hpp"
#include "Renderer/D3D12/Resource_DX12.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"

namespace Butterfly
{
	BFView::BFView(const DX12Resource& resource)
		: m_resource(resource)
	{
	}

	///
	/// BFRenderTargetView
	///

	BFRenderTargetView::BFRenderTargetView(const DX12Resource& resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc)
		: m_desc(desc), BFView(resource)
	{
		m_viewIndex = DX12API()->DescriptorAllocatorRtv()->CreateRtv(resource, &desc);
		m_handle = DX12API()->DescriptorAllocatorRtv()->CpuHandleFromRtvHandle(m_viewIndex);
	}

	BFRenderTargetView::~BFRenderTargetView()
	{
		DX12API()->DescriptorAllocatorRtv()->FreeHandle(m_viewIndex);
	}

	///
	/// BFDepthStencilView
	///

	BFDepthStencilView::BFDepthStencilView(const DX12Resource& resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
		: m_desc(desc), BFView(resource)
	{
		m_viewIndex = DX12API()->DescriptorAllocatorDsv()->CreateDsv(resource, &desc);
		m_handle = DX12API()->DescriptorAllocatorDsv()->CpuHandleFromDsvHandle(m_viewIndex);
	}

	BFDepthStencilView::~BFDepthStencilView()
	{
		DX12API()->DescriptorAllocatorDsv()->FreeHandle(m_viewIndex);
	}

	///
	/// BFShaderResourceView
	///

	BFShaderResourceView::BFShaderResourceView(const DX12Resource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
		: m_desc(desc), BFView(resource)
	{
		m_viewIndex = DX12API()->DescriptorAllocatorSrvCbvUav()->CreateSrv(resource, &desc);
	}

	BFShaderResourceView::~BFShaderResourceView()
	{
		DX12API()->DescriptorAllocatorSrvCbvUav()->FreeHandle(m_viewIndex);
	}

	///
	/// BFUniformBufferView
	///

	BFUniformBufferView::BFUniformBufferView(const DX12Resource& resource, uint32_t sizeInBytes)
		: m_sizeInBytes(sizeInBytes), BFView(resource)
	{
		m_viewIndex = DX12API()->DescriptorAllocatorSrvCbvUav()->CreateCbv(sizeInBytes, resource);
	}

	BFUniformBufferView::~BFUniformBufferView()
	{
		DX12API()->DescriptorAllocatorSrvCbvUav()->FreeHandle(m_viewIndex);
	}
}