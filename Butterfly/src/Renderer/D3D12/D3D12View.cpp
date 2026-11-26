#include "Renderer/D3D12/D3D12View.hpp"
#include "Renderer/D3D12/D3D12DescriptorAllocator.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"

namespace Butterfly
{
	namespace Utils
	{
		D3D12SubresourceRange RangeFromDesc(const D3D12_SHADER_RESOURCE_VIEW_DESC& d)
		{
			switch (d.ViewDimension)
			{
			case D3D12_SRV_DIMENSION_TEXTURE1D:        return D3D12SubresourceRange().MipRange(d.Texture1D.MostDetailedMip, d.Texture1D.MipLevels).SliceRange(0, 1);
			case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:   return D3D12SubresourceRange().MipRange(d.Texture1DArray.MostDetailedMip, d.Texture1DArray.MipLevels).SliceRange(d.Texture1DArray.FirstArraySlice, d.Texture1DArray.ArraySize);
			case D3D12_SRV_DIMENSION_TEXTURE2D:        return D3D12SubresourceRange().MipRange(d.Texture2D.MostDetailedMip, d.Texture2D.MipLevels).SliceRange(0, 1);
			case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:   return D3D12SubresourceRange().MipRange(d.Texture2DArray.MostDetailedMip, d.Texture2DArray.MipLevels).SliceRange(d.Texture2DArray.FirstArraySlice, d.Texture2DArray.ArraySize);
			case D3D12_SRV_DIMENSION_TEXTURE2DMS:      return D3D12SubresourceRange().MipRange(0, 1).SliceRange(0, 1);
			case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY: return D3D12SubresourceRange().MipRange(0, 1).SliceRange(d.Texture2DMSArray.FirstArraySlice, d.Texture2DMSArray.ArraySize);
			case D3D12_SRV_DIMENSION_TEXTURE3D:        return D3D12SubresourceRange().MipRange(d.Texture3D.MostDetailedMip, d.Texture3D.MipLevels).SliceRange(0, 1);
			case D3D12_SRV_DIMENSION_TEXTURECUBE:      return D3D12SubresourceRange().MipRange(d.TextureCube.MostDetailedMip, d.TextureCube.MipLevels).SliceRange(0, 6);
			case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY: return D3D12SubresourceRange().MipRange(d.TextureCubeArray.MostDetailedMip, d.TextureCubeArray.MipLevels).SliceRange(d.TextureCubeArray.First2DArrayFace, d.TextureCubeArray.NumCubes * 6);
			default:                                   return D3D12SubresourceRange(); // buffers, acceleration structures all use full range.
			}
		}

		D3D12SubresourceRange RangeFromDesc(const D3D12_UNORDERED_ACCESS_VIEW_DESC& d)
		{
			switch (d.ViewDimension)
			{
			case D3D12_UAV_DIMENSION_TEXTURE1D:      return D3D12SubresourceRange().MipRange(d.Texture1D.MipSlice, 1).SliceRange(0, 1);
			case D3D12_UAV_DIMENSION_TEXTURE1DARRAY: return D3D12SubresourceRange().MipRange(d.Texture1DArray.MipSlice, 1).SliceRange(d.Texture1DArray.FirstArraySlice, d.Texture1DArray.ArraySize);
			case D3D12_UAV_DIMENSION_TEXTURE2D:      return D3D12SubresourceRange().MipRange(d.Texture2D.MipSlice, 1).SliceRange(0, 1);
			case D3D12_UAV_DIMENSION_TEXTURE2DARRAY: return D3D12SubresourceRange().MipRange(d.Texture2DArray.MipSlice, 1).SliceRange(d.Texture2DArray.FirstArraySlice, d.Texture2DArray.ArraySize);
			case D3D12_UAV_DIMENSION_TEXTURE3D:      return D3D12SubresourceRange().MipRange(d.Texture3D.MipSlice, 1).SliceRange(0, 1);
			default:                                 return D3D12SubresourceRange(); // buffers use full range.
			}
		}

		D3D12SubresourceRange RangeFromDesc(const D3D12_RENDER_TARGET_VIEW_DESC& d)
		{
			switch (d.ViewDimension)
			{
			case D3D12_RTV_DIMENSION_TEXTURE1D:        return D3D12SubresourceRange().MipRange(d.Texture1D.MipSlice, 1).SliceRange(0, 1);
			case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:   return D3D12SubresourceRange().MipRange(d.Texture1DArray.MipSlice, 1).SliceRange(d.Texture1DArray.FirstArraySlice, d.Texture1DArray.ArraySize);
			case D3D12_RTV_DIMENSION_TEXTURE2D:        return D3D12SubresourceRange().MipRange(d.Texture2D.MipSlice, 1).SliceRange(0, 1);
			case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:   return D3D12SubresourceRange().MipRange(d.Texture2DArray.MipSlice, 1).SliceRange(d.Texture2DArray.FirstArraySlice, d.Texture2DArray.ArraySize);
			case D3D12_RTV_DIMENSION_TEXTURE2DMS:      return D3D12SubresourceRange().MipRange(0, 1).SliceRange(0, 1);
			case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY: return D3D12SubresourceRange().MipRange(0, 1).SliceRange(d.Texture2DMSArray.FirstArraySlice, d.Texture2DMSArray.ArraySize);
			case D3D12_RTV_DIMENSION_TEXTURE3D:        return D3D12SubresourceRange().MipRange(d.Texture3D.MipSlice, 1).SliceRange(0, 1);
			default:                                   return D3D12SubresourceRange(); // buffers use full range.
			}
		}

		D3D12SubresourceRange RangeFromDesc(const D3D12_DEPTH_STENCIL_VIEW_DESC& d)
		{
			switch (d.ViewDimension)
			{
			case D3D12_DSV_DIMENSION_TEXTURE1D:        return D3D12SubresourceRange().MipRange(d.Texture1D.MipSlice, 1).SliceRange(0, 1);
			case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:   return D3D12SubresourceRange().MipRange(d.Texture1DArray.MipSlice, 1).SliceRange(d.Texture1DArray.FirstArraySlice, d.Texture1DArray.ArraySize);
			case D3D12_DSV_DIMENSION_TEXTURE2D:        return D3D12SubresourceRange().MipRange(d.Texture2D.MipSlice, 1).SliceRange(0, 1);
			case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:   return D3D12SubresourceRange().MipRange(d.Texture2DArray.MipSlice, 1).SliceRange(d.Texture2DArray.FirstArraySlice, d.Texture2DArray.ArraySize);
			case D3D12_DSV_DIMENSION_TEXTURE2DMS:      return D3D12SubresourceRange().MipRange(0, 1).SliceRange(0, 1);
			case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY: return D3D12SubresourceRange().MipRange(0, 1).SliceRange(d.Texture2DMSArray.FirstArraySlice, d.Texture2DMSArray.ArraySize);
			default:                                   return D3D12SubresourceRange(); // buffers use full range.
			}
		}
	}

	View::View(D3D12Resource& resource)
		: m_resource(resource)
	{
	}

	///
	/// RenderTargetView
	///

	RenderTargetView::RenderTargetView(D3D12Resource& resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc)
		: m_desc(desc), View(resource)
	{
		m_viewIndex = D3D12API()->DescriptorAllocatorRtv()->CreateRtv(resource, &desc);
		m_handle = D3D12API()->DescriptorAllocatorRtv()->CpuHandleFromRtvHandle(m_viewIndex);

		m_requiredState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		m_subresourceRange = Utils::RangeFromDesc(desc);
	}

	RenderTargetView::~RenderTargetView()
	{
		D3D12API()->DescriptorAllocatorRtv()->FreeHandle(m_viewIndex);
	}

	///
	/// DepthStencilView
	///

	DepthStencilView::DepthStencilView(D3D12Resource& resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
		: m_desc(desc), View(resource)
	{
		m_viewIndex = D3D12API()->DescriptorAllocatorDsv()->CreateDsv(resource, &desc);
		m_handle = D3D12API()->DescriptorAllocatorDsv()->CpuHandleFromDsvHandle(m_viewIndex);

		const bool depthReadOnly = desc.Flags & D3D12_DSV_FLAG_READ_ONLY_DEPTH;
		const bool stencilReadOnly = desc.Flags & D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		const bool hasStencil = (desc.Format == DXGI_FORMAT_D24_UNORM_S8_UINT || desc.Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT);

		m_requiredState = (depthReadOnly && (!hasStencil || stencilReadOnly))
    		? D3D12_RESOURCE_STATE_DEPTH_READ
    		: D3D12_RESOURCE_STATE_DEPTH_WRITE;

		m_subresourceRange = Utils::RangeFromDesc(desc);
	}

	DepthStencilView::~DepthStencilView()
	{
		D3D12API()->DescriptorAllocatorDsv()->FreeHandle(m_viewIndex);
	}

	///
	/// ShaderResourceView
	///

	ShaderResourceView::ShaderResourceView(D3D12Resource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
		: m_desc(desc), View(resource)
	{
		m_viewIndex = D3D12API()->DescriptorAllocatorSrvCbvUav()->CreateSrv(resource, &desc);

		m_requiredState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    	if (desc.ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)
		{
			m_requiredState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		}

		m_subresourceRange = Utils::RangeFromDesc(desc);
	}

	ShaderResourceView::~ShaderResourceView()
	{
		D3D12API()->DescriptorAllocatorSrvCbvUav()->FreeHandle(m_viewIndex);
	}

	///
	/// UniformBufferView
	///

	UniformBufferView::UniformBufferView(D3D12Resource& resource, uint32_t sizeInBytes, uint32_t offset)
		: m_sizeInBytes(sizeInBytes), View(resource)
	{
		m_viewIndex = D3D12API()->DescriptorAllocatorSrvCbvUav()->CreateCbv(sizeInBytes, resource, offset);
		m_offset = offset;

		m_requiredState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		m_subresourceRange = D3D12SubresourceRange().FullRange();
	}

	UniformBufferView::~UniformBufferView()
	{
		D3D12API()->DescriptorAllocatorSrvCbvUav()->FreeHandle(m_viewIndex);
	}


	///
	/// UnorderedAccessView
	/// 
	
	UnorderedAccessView::UnorderedAccessView(D3D12Resource& resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
		: m_desc(desc), View(resource)
	{
		m_viewIndex = D3D12API()->DescriptorAllocatorSrvCbvUav()->CreateUav(resource, &desc);

		m_requiredState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		m_subresourceRange = Utils::RangeFromDesc(desc);
	}

	UnorderedAccessView::~UnorderedAccessView()
	{
		D3D12API()->DescriptorAllocatorSrvCbvUav()->FreeHandle(m_viewIndex);
	}
}
