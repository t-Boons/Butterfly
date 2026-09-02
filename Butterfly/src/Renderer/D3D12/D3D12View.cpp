#include "Renderer/D3D12/D3D12View.hpp"
#include "Renderer/D3D12/D3D12DescriptorAllocator.hpp"
#include "Renderer/D3D12/D3D12Resource.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"

namespace Butterfly
{
	BFView::BFView(const D3D12Resource& resource)
		: m_resource(resource)
	{
	}

	///
	/// BFRenderTargetView
	///

	BFRenderTargetView::BFRenderTargetView(const D3D12Resource& resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc)
		: m_desc(desc), BFView(resource)
	{
		m_viewIndex = D3D12API()->DescriptorAllocatorRtv()->CreateRtv(resource, &desc);
		m_handle = D3D12API()->DescriptorAllocatorRtv()->CpuHandleFromRtvHandle(m_viewIndex);
	}

	BFRenderTargetView::~BFRenderTargetView()
	{
		D3D12API()->DescriptorAllocatorRtv()->FreeHandle(m_viewIndex);
	}

	///
	/// BFDepthStencilView
	///

	BFDepthStencilView::BFDepthStencilView(const D3D12Resource& resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
		: m_desc(desc), BFView(resource)
	{
		m_viewIndex = D3D12API()->DescriptorAllocatorDsv()->CreateDsv(resource, &desc);
		m_handle = D3D12API()->DescriptorAllocatorDsv()->CpuHandleFromDsvHandle(m_viewIndex);
	}

	BFDepthStencilView::~BFDepthStencilView()
	{
		D3D12API()->DescriptorAllocatorDsv()->FreeHandle(m_viewIndex);
	}

	///
	/// BFShaderResourceView
	///

	BFShaderResourceView::BFShaderResourceView(const D3D12Resource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
		: m_desc(desc), BFView(resource)
	{
		m_viewIndex = D3D12API()->DescriptorAllocatorSrvCbvUav()->CreateSrv(resource, &desc);
	}

	BFShaderResourceView::~BFShaderResourceView()
	{
		D3D12API()->DescriptorAllocatorSrvCbvUav()->FreeHandle(m_viewIndex);
	}

	///
	/// BFUniformBufferView
	///

	BFUniformBufferView::BFUniformBufferView(const D3D12Resource& resource, uint32_t sizeInBytes)
		: m_sizeInBytes(sizeInBytes), BFView(resource)
	{
		m_viewIndex = D3D12API()->DescriptorAllocatorSrvCbvUav()->CreateCbv(sizeInBytes, resource);
	}

	BFUniformBufferView::~BFUniformBufferView()
	{
		D3D12API()->DescriptorAllocatorSrvCbvUav()->FreeHandle(m_viewIndex);
	}
}
