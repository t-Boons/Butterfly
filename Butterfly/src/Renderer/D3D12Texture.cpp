#include "Renderer/D3D12Texture.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12Resource.hpp"
#include "Renderer/D3D12/D3D12View.hpp"
#include "Renderer/D3D12/D3D12GraphicsCommands.hpp"

namespace Butterfly
{
	namespace Utils
	{
		inline uint32_t MaxMips(uint32_t width, uint32_t height)
		{
			return 1 + static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))));
		}


		inline DXGI_FORMAT SRVCompatibleFormat(DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT_D32_FLOAT:
				return DXGI_FORMAT_R32_FLOAT;
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
				return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			default:
				return format; // Return the original format if no compatible SRV format exists
			}
		}
		inline DXGI_FORMAT GetSRGBFormat(DXGI_FORMAT format)
		{
			switch (format)
			{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
				return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			case DXGI_FORMAT_BC1_UNORM:
				return DXGI_FORMAT_BC1_UNORM_SRGB;
			case DXGI_FORMAT_BC2_UNORM:
				return DXGI_FORMAT_BC2_UNORM_SRGB;
			case DXGI_FORMAT_BC3_UNORM:
				return DXGI_FORMAT_BC3_UNORM_SRGB;
			case DXGI_FORMAT_BC7_UNORM:
				return DXGI_FORMAT_BC7_UNORM_SRGB;
			default:
				return format; // Return the original format if no sRGB equivalent exists
			}
		}

		inline const D3D12_RENDER_TARGET_VIEW_DESC CreateRTVDescFromHWTextureDesc(const BFTextureDesc& desc, const BFRTVTextureViewInfo& info)
		{
			BF_PROFILE_EVENT();

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = desc.Format;

			if (desc.SRGBHint)
			{
				rtvDesc.Format = GetSRGBFormat(rtvDesc.Format);
			}

			if (info.Type == BFTextureUsage::Texture2D)
			{
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				rtvDesc.Texture2D.MipSlice = info.Range.FirstMip;
			}

			if (info.Type == BFTextureUsage::Texture2DArray || info.Type == BFTextureUsage::Cubemap)
			{
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;

				rtvDesc.Texture2DArray.MipSlice = info.Range.FirstMip;
				rtvDesc.Texture2DArray.FirstArraySlice = info.Range.FirstSlice;
				rtvDesc.Texture2DArray.ArraySize = info.Range.SliceCount;
			}

			return rtvDesc;
		}

		inline const D3D12_DEPTH_STENCIL_VIEW_DESC CreateDsvDescFromHWTextureDesc(const BFTextureDesc& desc, const BFDSVTextureViewInfo& info)
		{
			BF_PROFILE_EVENT();

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};

			dsvDesc.Format = desc.Format;

			if (info.Type == BFTextureUsage::Texture2D)
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
				dsvDesc.Texture2D.MipSlice = info.Range.FirstMip;
			}

			if (info.Type == BFTextureUsage::Texture2DArray || info.Type == BFTextureUsage::Cubemap)
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

				dsvDesc.Texture2DArray.MipSlice = info.Range.FirstMip;
				dsvDesc.Texture2DArray.FirstArraySlice = info.Range.FirstSlice;
				dsvDesc.Texture2DArray.ArraySize = info.Range.SliceCount;
			}

			return dsvDesc;
		}

		inline const D3D12_SHADER_RESOURCE_VIEW_DESC CreateSrvFromHWTextureDesc(const BFTextureDesc& desc, const BFSRVTextureViewInfo& info)
		{
			BF_PROFILE_EVENT();

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = SRVCompatibleFormat(desc.Format);
			if (info.SRGB)
			{
				srvDesc.Format = GetSRGBFormat(srvDesc.Format);
			}
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = info.Range.FirstMip;
			srvDesc.Texture2D.MipLevels = info.Range.MipCount;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

			if (info.Type == BFTextureUsage::Texture2DArray)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.Texture2DArray.MostDetailedMip = info.Range.FirstMip;
				srvDesc.Texture2DArray.MipLevels = info.Range.MipCount;
				srvDesc.Texture2DArray.FirstArraySlice = info.Range.FirstSlice;
				srvDesc.Texture2DArray.ArraySize = info.Range.SliceCount;
				srvDesc.Texture2DArray.PlaneSlice = 0;
				srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
			}

			if (info.Type == BFTextureUsage::Cubemap)
			{
				BF_CORE_ASSERT(desc.ArraySize == 6, "Cubemap must have 6 faces.");
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MostDetailedMip = info.Range.FirstMip;
				srvDesc.TextureCube.MipLevels = info.Range.MipCount;
				srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
			}

			if (info.Type == BFTextureUsage::Texture3D)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				srvDesc.Texture3D.MostDetailedMip = info.Range.FirstMip;
				srvDesc.Texture3D.MipLevels = info.Range.MipCount;
				srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
			}

			return srvDesc;
		}

		inline const D3D12_UNORDERED_ACCESS_VIEW_DESC CreateUavFromHWTextureDesc(const BFTextureDesc& desc, const BFUAVTextureViewInfo& info)
		{
			BF_PROFILE_EVENT();
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = desc.Format;

			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = info.Range.FirstMip;
			uavDesc.Texture2D.PlaneSlice = 0;

			if (info.Type == BFTextureUsage::Texture2DArray)
			{
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2DArray.ArraySize = info.Range.SliceCount;
				uavDesc.Texture2DArray.FirstArraySlice = info.Range.FirstSlice;
				uavDesc.Texture2DArray.MipSlice = info.Range.FirstMip;
			}
			if (info.Type == BFTextureUsage::Cubemap)
			{
				BF_CORE_ASSERT(desc.ArraySize == 6, "Cubemap must have 6 faces.");
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2DArray.ArraySize = info.Range.SliceCount;
				uavDesc.Texture2DArray.FirstArraySlice = info.Range.FirstSlice;
				uavDesc.Texture2DArray.MipSlice = info.Range.FirstMip;
			}
			if (info.Type == BFTextureUsage::Texture3D)
			{
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				uavDesc.Texture3D.MipSlice = info.Range.FirstMip;
				uavDesc.Texture3D.FirstWSlice = 0;
				uavDesc.Texture3D.WSize = desc.Depth;
			}
			return uavDesc;
		}

		D3D12_RESOURCE_STATES GetInitialResourceState(uint32_t v)
		{
			if (v & BFTextureDesc::RenderTargettable) return D3D12_RESOURCE_STATE_RENDER_TARGET;
			if (v & BFTextureDesc::DepthStencilable) return D3D12_RESOURCE_STATE_DEPTH_WRITE;
			if (v & BFTextureDesc::UnorderedAccess)  return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}
	}

	BFTexture::BFTexture(const BFTextureDesc& desc)
		: m_desc(desc)
	{
		BF_CORE_ASSERT(desc.Width > 0 && desc.Height > 0, "Texture width and height must be greater than 0");
		BF_CORE_ASSERT(desc.ArraySize > 0, "Texture array size must be greater than 0");
		BF_CORE_ASSERT(desc.ViewTypes != BFTextureDesc::None, "Texture view types must be specified");

		if (m_desc.NumMips == BF_ALL)
		{
			m_desc.NumMips = Utils::MaxMips(desc.Width, desc.Height);
		}

		const uint32_t v = m_desc.ViewTypes;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

		if (v & BFTextureDesc::RenderTargettable) flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		if (v & BFTextureDesc::DepthStencilable) flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		if (v & BFTextureDesc::UnorderedAccess || m_desc.NumMips > 1)
			flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		if ((v & BFTextureDesc::DepthStencilable) && !(v & BFTextureDesc::ShaderResource))
			flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

		D3D12ResourceDescription resourceDesc;

		if (m_desc.Dimention == BFTextureDimention::Texture3D)
			resourceDesc = D3D12ResourceDescription::Texture3D(m_desc.Format, m_desc.Width, m_desc.Height, m_desc.Depth, m_desc.NumMips);
		else if(m_desc.Dimention == BFTextureDimention::Texture2D)
			resourceDesc = D3D12ResourceDescription::Texture(m_desc.Format, m_desc.Width, m_desc.Height, m_desc.ArraySize, m_desc.NumMips);

		resourceDesc.HeapType(D3D12_HEAP_TYPE_DEFAULT);
		resourceDesc.Flags(flags);
		if (v & (BFTextureDesc::RenderTargettable | BFTextureDesc::DepthStencilable))
		{
			float col[] = { 0,0,0,0 };
			resourceDesc.ClearColor(col, m_desc.Format);
			resourceDesc.ClearDepth(m_desc.Format);
		}

		resourceDesc.InitialState(Utils::GetInitialResourceState(m_desc.ViewTypes));
		resourceDesc.SetName(m_desc.DebugName);
		m_resource = new D3D12Resource(resourceDesc);
	}

	BFTexture::~BFTexture()
	{
		BF_PROFILE_EVENT();
		FREE(m_resource);
	}

	D3D12_CLEAR_VALUE BFTexture::ClearValue() const
	{
		return m_resource->ClearValue();
	}

	const BFRenderTargetView& BFTexture::RTV()
	{
		BFRTVTextureViewInfo info;
		info.Range.FirstSlice = 0;
		info.Range.FirstMip = 0;
		info.Range.SliceCount = BF_ALL;
		info.Range.MipCount = BF_ALL;
		return RTV(info);
	}

	const BFRenderTargetView &BFTexture::RTV(BFRTVTextureViewInfo info)
	{
		const uint32_t mipCount   = m_resource->MipCount();
		const uint32_t sliceCount = m_resource->ArraySize();

		D3D12SubresourceRange& range = info.Range;

		BF_CORE_ASSERT(range.FirstMip < mipCount, "First mip out of bounds for RTV.");
		BF_CORE_ASSERT(range.FirstSlice < sliceCount, "First slice out of bounds for RTV.");
		BF_CORE_ASSERT(range.MipCount == BF_ALL || range.FirstMip + range.MipCount <= mipCount, "Mip range out of bounds for RTV.");
		BF_CORE_ASSERT(range.SliceCount == BF_ALL || range.FirstSlice + range.SliceCount <= sliceCount, "Slice range out of bounds for RTV.");

		// Resolve BF_ALL to the real remaining count.
		range.MipCount   = std::min(range.MipCount, mipCount - range.FirstMip);
		range.SliceCount = std::min(range.SliceCount, sliceCount - range.FirstSlice);
		uint64_t key = 1;
		Utils::SumHash(key, info.Range.FirstSlice);
		Utils::SumHash(key, info.Range.FirstMip);
		Utils::SumHash(key, info.Range.SliceCount);
		Utils::SumHash(key, info.Range.MipCount);

		auto it = m_rtvSlices.find(key);

		if (it == m_rtvSlices.end())
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = Utils::CreateRTVDescFromHWTextureDesc(m_desc, info);
			m_rtvSlices[key] = MakeRef<BFRenderTargetView>(*m_resource, rtvDesc);
			it = m_rtvSlices.find(key);
		}

		return *it->second;
	}

	const BFDepthStencilView& BFTexture::DSV()
	{
		BFDSVTextureViewInfo info;
		info.Range.FirstSlice = 0;
		info.Range.FirstMip = 0;
		info.Range.SliceCount = BF_ALL;
		info.Range.MipCount = BF_ALL;
		return DSV(info);
	}

	const BFDepthStencilView& BFTexture::DSV(BFDSVTextureViewInfo info)
	{
		BF_CORE_ASSERT(info.Range.FirstSlice < m_desc.ArraySize, "Slice index out of bounds for DSV array slice.");

		const uint32_t mipCount   = m_resource->MipCount();
		const uint32_t sliceCount = m_resource->ArraySize();

		// Resolve BF_ALL to the real remaining count.
		info.Range.MipCount   = std::min(info.Range.MipCount, mipCount - info.Range.FirstMip);
		info.Range.SliceCount = std::min(info.Range.SliceCount, sliceCount - info.Range.FirstSlice);

		uint64_t key = 1;
		Utils::SumHash(key, info.Range.FirstSlice);
		Utils::SumHash(key, info.Range.FirstMip);
		Utils::SumHash(key, info.Range.SliceCount);
		Utils::SumHash(key, info.Range.MipCount);

		auto it = m_dsvSlices.find(key);

		if (it == m_dsvSlices.end())
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = Utils::CreateDsvDescFromHWTextureDesc(m_desc, info);
			m_dsvSlices[key] = MakeRef<BFDepthStencilView>(*m_resource, dsvDesc);
			it = m_dsvSlices.find(key);
		}

		return *it->second;
	}

	const BFShaderResourceView& BFTexture::SRV()
	{
		BFSRVTextureViewInfo info;
		info.Range.FirstSlice = 0;
		info.Range.SliceCount = BF_ALL;
		info.Range.FirstMip = 0;
		info.Range.MipCount = BF_ALL;
		info.SRGB = m_desc.SRGBHint;
		return SRV(info);
	}

	const BFShaderResourceView& BFTexture::SRV(BFSRVTextureViewInfo info)
	{
		const uint32_t mipCount   = m_resource->MipCount();
		const uint32_t sliceCount = m_resource->ArraySize();

		D3D12SubresourceRange& range = info.Range;

		BF_CORE_ASSERT(range.FirstMip < mipCount, "First mip out of bounds for SRV.");
		BF_CORE_ASSERT(range.FirstSlice < sliceCount, "First slice out of bounds for SRV.");
		BF_CORE_ASSERT(range.MipCount == BF_ALL || range.FirstMip + range.MipCount <= mipCount, "Mip range out of bounds for SRV.");
		BF_CORE_ASSERT(range.SliceCount == BF_ALL || range.FirstSlice + range.SliceCount <= sliceCount, "Slice range out of bounds for SRV.");

		// Resolve BF_ALL to the real remaining count.
		range.MipCount   = std::min(range.MipCount, mipCount - range.FirstMip);
		range.SliceCount = std::min(range.SliceCount, sliceCount - range.FirstSlice);

		uint64_t key = 1;
		Utils::SumHash(key, range.FirstMip);
		Utils::SumHash(key, range.MipCount);
		Utils::SumHash(key, range.FirstSlice);
		Utils::SumHash(key, range.SliceCount);
		Utils::SumHash(key, static_cast<uint32_t>(info.SRGB));
		Utils::SumHash(key, static_cast<uint32_t>(info.Type));

		auto it = m_srvSlices.find(key);
		if (it == m_srvSlices.end())
		{
			const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = Utils::CreateSrvFromHWTextureDesc(m_desc, info);
			it = m_srvSlices.emplace(key, MakeRef<BFShaderResourceView>(*m_resource, srvDesc)).first;
		}

		return *it->second;
	}

	const BFUnorderedAccessView& BFTexture::UAV(BFUAVTextureViewInfo info)
	{
		const uint32_t mipCount   = m_resource->MipCount();
		const uint32_t sliceCount = m_resource->ArraySize();

		D3D12SubresourceRange& range = info.Range;

		BF_CORE_ASSERT(range.FirstMip < mipCount, "Mip out of bounds for UAV.");
		BF_CORE_ASSERT(range.MipCount == 1, "UAV can only target one mip.");
		BF_CORE_ASSERT(range.FirstSlice < sliceCount, "First slice out of bounds for UAV.");
		BF_CORE_ASSERT(range.SliceCount == BF_ALL || range.FirstSlice + range.SliceCount <= sliceCount, "Slice range out of bounds for UAV.");

		// Resolve SliceCount into the real number of slices remaining
		range.SliceCount = std::min(range.SliceCount, sliceCount - range.FirstSlice);

		uint64_t key = 1;
		Utils::SumHash(key, range.FirstMip);
		Utils::SumHash(key, range.FirstSlice);
		Utils::SumHash(key, range.SliceCount);
		Utils::SumHash(key, static_cast<uint32_t>(info.Type));

		auto it = m_uavSlices.find(key);
		if (it == m_uavSlices.end())
		{
			const D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = Utils::CreateUavFromHWTextureDesc(m_desc, info);
			it = m_uavSlices.emplace(key, MakeRef<BFUnorderedAccessView>(*m_resource, uavDesc)).first;
		}

		return *it->second;
	}

	const BFUnorderedAccessView& BFTexture::UAV()
	{
		BFUAVTextureViewInfo info;
		info.Range.FirstSlice = 0;
		info.Range.SliceCount = BF_ALL;
		info.Range.FirstMip = 0;
		info.Range.MipCount = BF_ALL;
		return UAV(info);
	}

	BFTextureReadback::BFTextureReadback()
	{
		m_readbackBuffer = new D3D12Resource(
			D3D12ResourceDescription::Buffer(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
			.HeapType(D3D12_HEAP_TYPE_READBACK)
			.InitialState(D3D12_RESOURCE_STATE_COPY_DEST)
			.SetName("Object Picker Pixel Readback"));
	}

	BFTextureReadback::~BFTextureReadback()
	{
		BF_PROFILE_EVENT();
		FREE(m_readbackBuffer);
	}

	bool BFTextureReadback::RequestPixel(const glm::uvec2& pixel)
	{
		if (m_hasRequest || m_waitingForResult)
		{
			return false;
		}

		m_requestedPixel = pixel;
		m_hasRequest = true;
		return true;
	}

	void BFTextureReadback::SetFenceValue(uint64_t fenceValue)
	{
		if (!m_copyRecorded)
		{
			return;
		}

		m_fenceValue = fenceValue;
		m_waitingForResult = true;
		m_copyRecorded = false;
	}

	void BFTextureReadback::Flush()
	{
		m_waitingForResult = false;
		m_copyRecorded = false;
	}

	bool BFTextureReadback::TryGetResult(uint64_t completedFenceValue, uint32_t& out)
	{
		if (!m_waitingForResult || completedFenceValue < m_fenceValue)
		{
			return false;
		}

		D3D12_RANGE readRange{ 0, sizeof(uint32_t) };
		void* mapped = nullptr;
		ThrowIfFailed(m_readbackBuffer->HWResource()->Map(0, &readRange, &mapped));
		out = *static_cast<const uint32_t*>(mapped);

		D3D12_RANGE writtenRange{ 0, 0 };
		m_readbackBuffer->HWResource()->Unmap(0, &writtenRange);

		m_waitingForResult = false;
		m_hasRequest = false;
		return true;
	}

	bool BFTextureReadback::Readback(D3D12CommandList& list, BFTexture& texture)
	{
		if (!m_hasRequest)
		{
			return false;
		}

		// Cancel the request if it is out of bounds.
		if (m_requestedPixel.x >= texture.Width() || m_requestedPixel.y >= texture.Height())
		{
			BF_CORE_LOG_WARN("Requested pixel (%d, %d) is out of bounds for texture of size (%d, %d). Cancelling readback request.", m_requestedPixel.x, m_requestedPixel.y, texture.Width(), texture.Height());
			m_hasRequest = false;
			m_waitingForResult = false;
			return false;
		}

		D3D12Resource* src = texture.Resource();

		src->TransitionAll(list, D3D12_RESOURCE_STATE_COPY_SOURCE);


		D3D12_TEXTURE_COPY_LOCATION source{};
		source.pResource = src->HWResource();
		source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		source.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION destination{};
		destination.pResource = m_readbackBuffer->HWResource();
		destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		destination.PlacedFootprint.Offset = 0;
		destination.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R32_UINT;
		destination.PlacedFootprint.Footprint.Width = 1;
		destination.PlacedFootprint.Footprint.Height = 1;
		destination.PlacedFootprint.Footprint.Depth = 1;
		destination.PlacedFootprint.Footprint.RowPitch = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;

		D3D12_BOX sourceBox{};
		sourceBox.left = static_cast<UINT>(m_requestedPixel.x);
		sourceBox.top = static_cast<UINT>(m_requestedPixel.y);
		sourceBox.front = 0;
		sourceBox.right = sourceBox.left + 1;
		sourceBox.bottom = sourceBox.top + 1;
		sourceBox.back = 1;

		list.List()->CopyTextureRegion(&destination, 0, 0, 0, &source, &sourceBox);

		m_copyRecorded = true;
		return true;
	}
}
