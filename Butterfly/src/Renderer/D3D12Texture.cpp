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
		inline void CopyGPUTexturesToCubemap(const std::array<RefPtr<BFTexture>, 6>& cubemapFacesSource, D3D12Resource* cubemap)
		{
			D3D12CommandList list(D3D12_COMMAND_LIST_TYPE_COPY);

			for (uint32_t face = 0; face < 6; ++face)
			{
				if (!cubemapFacesSource[face])
				{
					BF_CORE_LOG_WARN("Face %d of cubemap is null or has no resource. Skipping copy.", face);
					continue;
				}

				D3D12_TEXTURE_COPY_LOCATION src = {};
				src.pResource = cubemapFacesSource[face]->Resource()->HwResource;
				src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				src.SubresourceIndex = 0;

				D3D12_TEXTURE_COPY_LOCATION dst = {};
				dst.pResource = cubemap->HwResource;
				dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				dst.SubresourceIndex = face;

				list.List()->CopyTextureRegion(
					&dst,
					0, 0, 0,
					&src,
					nullptr
				);
			}

			list.Close();

			D3D12API()->Queue(QueueType::Copy)->Execute(list);
			D3D12API()->Queue(QueueType::Copy)->WaitForFence();
		}


		inline void CopyTextureFromCPU(D3D12Resource* newTexture, const BFTextureDesc& desc)
		{
			BF_CORE_ASSERT(desc.UploadData.CPUCopySource, "If you want to copy data from CPU, you need to provide data in the BFTextureDesc::UploadData.CPUCopySource field.");


			const D3D12_RESOURCE_DESC textureDesc = newTexture->HwResource->GetDesc();
			const uint32_t numSubresources = textureDesc.DepthOrArraySize;

			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(numSubresources);
			std::vector<UINT> numRows(numSubresources);
			std::vector<UINT64> rowSizes(numSubresources);

			uint64_t totalSize = 0;

			D3D12API()->Device()->GetCopyableFootprints(&textureDesc, 0, numSubresources, 0, footprints.data(), numRows.data(), rowSizes.data(), &totalSize);

			D3D12Resource* uploadResource = DX12ResourceBuilder()
				.HeapType(D3D12_HEAP_TYPE_UPLOAD)
				.InitialState(D3D12_RESOURCE_STATE_COPY_SOURCE)
				.Buffer(totalSize)
				.SetName("Intermediate texture.")
				.Create();


			const uint32_t sourceRowPitch = desc.Width * GetBytesPerPixel(desc.Format);

			for (uint32_t slice = 0; slice < numSubresources; ++slice)
			{
				const uint8_t* src = static_cast<const uint8_t*>(desc.UploadData.CPUCopySource) + slice * desc.Height * sourceRowPitch;
				const auto& footprint = footprints[slice];


				const uint32_t dstRowPitch = footprint.Footprint.RowPitch;
				const uint32_t copyRowSize = static_cast<uint32_t>(rowSizes[slice]);

				for (uint32_t y = 0; y < desc.Height; ++y)
				{
					uploadResource->Write(src + y * sourceRowPitch, copyRowSize, static_cast<uint32_t>(footprint.Offset + y * dstRowPitch));
				}
			}

			D3D12CommandList list(D3D12_COMMAND_LIST_TYPE_COPY);

			for (uint32_t slice = 0; slice < numSubresources; ++slice)
			{
				D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
				srcLocation.pResource = uploadResource->HwResource;
				srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				srcLocation.PlacedFootprint = footprints[slice];

				D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
				dstLocation.pResource = newTexture->HwResource;
				dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				dstLocation.SubresourceIndex = slice;

				list.List()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
			}

			list.Close();

			D3D12API()->Queue(QueueType::Copy)->Execute(list);
			D3D12API()->Queue(QueueType::Copy)->WaitForFence();

			delete uploadResource;
		}

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

		inline const D3D12_RENDER_TARGET_VIEW_DESC CreateRTVDescFromHWTextureDesc(const BFTextureDesc& desc)
		{
			BF_PROFILE_EVENT();

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = desc.Format;

			if (desc.SRGBHint)
			{
				rtvDesc.Format = GetSRGBFormat(rtvDesc.Format);
			}


			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			return rtvDesc;
		}

		inline const D3D12_DEPTH_STENCIL_VIEW_DESC CreateDsvDescFromHWTextureDesc(const BFTextureDesc& desc, uint32_t slice = 0)
		{
			BF_PROFILE_EVENT();

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};

			dsvDesc.Format = desc.Format;

			if (desc.Type == BFTextureType::Texture2D)
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			}

			if (desc.Type == BFTextureType::Texture2DArray || desc.Type == BFTextureType::Cubemap)
			{
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

				dsvDesc.Texture2DArray.MipSlice = 0;
				dsvDesc.Texture2DArray.FirstArraySlice = slice;
				dsvDesc.Texture2DArray.ArraySize = 1;
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

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = desc.NumMips;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

			if (desc.Type == BFTextureType::Texture2DArray)
			{
				BF_CORE_ASSERT(desc.ArraySize > 1, "Texture2DArray must have ArraySize > 1.");
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.Texture2DArray.ArraySize = info.LastSlice - info.FirstSlice + 1;
				srvDesc.Texture2DArray.FirstArraySlice = info.FirstSlice;
				srvDesc.Texture2DArray.MostDetailedMip = 0;
				srvDesc.Texture2DArray.PlaneSlice = 0;
				srvDesc.Texture2DArray.MipLevels = desc.NumMips;
			}

			if (desc.Type == BFTextureType::Cubemap)
			{
				BF_CORE_ASSERT(desc.ArraySize == 6, "Cubemap must have 6 faces.");
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MostDetailedMip = 0;
				srvDesc.TextureCube.MipLevels = desc.NumMips;
				srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
			}
			if (desc.Type == BFTextureType::Texture3D)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				srvDesc.Texture3D.MostDetailedMip = 0;
				srvDesc.Texture3D.MipLevels = desc.NumMips;
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
			uavDesc.Texture2D.MipSlice = info.MipSlice;
			uavDesc.Texture2D.PlaneSlice = 0;

			if (desc.Type == BFTextureType::Texture2DArray)
			{
				BF_CORE_ASSERT(desc.ArraySize > 1, "Texture2DArray must have ArraySize > 1.");
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2DArray.ArraySize = info.LastSlice - info.FirstSlice + 1;
				uavDesc.Texture2DArray.FirstArraySlice = info.FirstSlice;
				uavDesc.Texture2DArray.MipSlice = info.MipSlice;
			}
			if (desc.Type == BFTextureType::Cubemap)
			{
				BF_CORE_ASSERT(desc.ArraySize == 6, "Cubemap must have 6 faces.");
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2DArray.ArraySize = info.LastSlice - info.FirstSlice + 1;
				uavDesc.Texture2DArray.FirstArraySlice = info.FirstSlice;
				uavDesc.Texture2DArray.MipSlice = info.MipSlice;
			}
			if (desc.Type == BFTextureType::Texture3D)
			{
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				uavDesc.Texture3D.MipSlice = info.MipSlice;
				uavDesc.Texture3D.FirstWSlice = 0;
				uavDesc.Texture3D.WSize = desc.Depth;
			}
			return uavDesc;
		}
	}

	BFTexture::BFTexture(const BFTextureDesc& desc)
		: m_desc(desc)
	{
		BF_CORE_ASSERT(desc.Width > 0 && desc.Height > 0, "Texture width and height must be greater than 0");
		BF_CORE_ASSERT(desc.ArraySize > 0, "Texture array size must be greater than 0");
		BF_CORE_ASSERT(desc.ViewTypes != BFTextureDesc::None, "Texture view types must be specified");

		if (m_desc.NumMips == 0)
		{
			m_desc.NumMips = Utils::MaxMips(desc.Width, desc.Height);
		}

		if (m_desc.ViewTypes & BFTextureDesc::ViewType::RenderTargettable)
		{
			BF_CORE_ASSERT(m_desc.Type != BFTextureType::Cubemap, "RenderTargettable textures cannot be Cubemaps.");
			BF_CORE_ASSERT(m_desc.Type != BFTextureType::Texture2DArray, "RenderTargettable textures cannot be Texture2DArray.");
			BF_CORE_ASSERT(m_desc.ArraySize == 1, "RenderTargettable textures cannot be Texture2DArray.");

			float col[] = { 0,0,0,0 };
			m_resource = DX12ResourceBuilder()
				.HeapType(D3D12_HEAP_TYPE_DEFAULT)
				.RenderTarget(m_desc.Format, m_desc.Width, m_desc.Height)
				.ClearColor(col, m_desc.Format)
				.InitialState(D3D12_RESOURCE_STATE_RENDER_TARGET)
				.SetName(m_desc.DebugName)
				.Create();
		}
		else if (m_desc.ViewTypes & BFTextureDesc::ViewType::DepthStencilable)
		{
			float col[] = { 0,0,0,0 };
			m_resource = DX12ResourceBuilder()
				.HeapType(D3D12_HEAP_TYPE_DEFAULT)
				.DepthStencil(m_desc.Format, m_desc.Width, m_desc.Height, m_desc.ArraySize)
				.ClearDepth(m_desc.Format)
				.InitialState(D3D12_RESOURCE_STATE_DEPTH_READ)
				.SetName(m_desc.DebugName)
				.Create();
		}
		else if (m_desc.ViewTypes & BFTextureDesc::ViewType::ShaderResource || m_desc.ViewTypes & BFTextureDesc::ViewType::UnorderedAccess)
		{
			float col[] = { 0,0,0,0 };

			DX12ResourceBuilder builder;
			builder.HeapType(D3D12_HEAP_TYPE_DEFAULT);

			if (m_desc.Type == BFTextureType::Texture3D)
				builder.Texture3D(m_desc.Format, m_desc.Width, m_desc.Height, m_desc.Depth, m_desc.NumMips);
			else
				builder.Texture(m_desc.Format, m_desc.Width, m_desc.Height, m_desc.ArraySize, m_desc.NumMips);

			if (m_desc.UploadData.CPUCopySource || m_desc.UploadData.GPUCopySource || m_desc.UploadData.HasValidFace() || m_desc.ViewTypes & BFTextureDesc::ViewType::UnorderedAccess)
				builder.InitialState(D3D12_RESOURCE_STATE_COMMON);
			else
				builder.InitialState(D3D12_RESOURCE_STATE_GENERIC_READ);

			if (m_desc.ViewTypes & BFTextureDesc::ViewType::UnorderedAccess || m_desc.NumMips > 1)
				builder.Flags(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			builder.SetName(m_desc.DebugName);
			m_resource = builder.Create();
		}

		if (m_desc.UploadData.CPUCopySource)
		{
			Utils::CopyTextureFromCPU(m_resource, m_desc);
		}

		if (m_desc.Type == BFTextureType::Cubemap && m_desc.UploadData.HasValidFace())
		{
			BF_CORE_ASSERT(m_desc.ArraySize == 6, "Cubemap must have 6 faces.");
			Utils::CopyGPUTexturesToCubemap(m_desc.UploadData.CubemapFacesSource, m_resource);
		}


		if (m_desc.ViewTypes & BFTextureDesc::ViewType::RenderTargettable)
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = Utils::CreateRTVDescFromHWTextureDesc(m_desc);
			m_rtv = new BFRenderTargetView(*m_resource, rtvDesc);
		}


		if (m_desc.NumMips > 1)
		{
			D3D12CommandList list;
			GraphicsCommands::GenerateMips(list, *this);
			list.Close();
			D3D12API()->Queue(QueueType::Direct)->Execute(list);
			D3D12API()->Queue(QueueType::Direct)->WaitForFence();
		}
	}

	BFTexture::~BFTexture()
	{
		BF_PROFILE_EVENT();

		FREE(m_rtv);
		FREE(m_resource);
	}

	const D3D12_CLEAR_VALUE* BFTexture::ClearValue() const
	{
		return &m_resource->ClearValue;
	}

	const BFRenderTargetView& BFTexture::RTV() const
	{
		BF_CORE_ASSERT(m_rtv != nullptr, "%s", "Texture does not have RTV, the BFTexture::Desc::Flag needs to have BFTexture::Desc::RenderTargettable set.");
		return *m_rtv;
	}

	const BFDepthStencilView& BFTexture::DSV()
	{
		return DSVArraySlice(0);
	}

	const BFDepthStencilView& BFTexture::DSVArraySlice(uint32_t slice)
	{
		BF_CORE_ASSERT(slice < m_desc.ArraySize, "Slice index out of bounds for DSV array slice.");

		auto it = m_dsvSlices.find(slice);

		if (it == m_dsvSlices.end())
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = Utils::CreateDsvDescFromHWTextureDesc(m_desc, slice);
			m_dsvSlices[slice] = MakeRef<BFDepthStencilView>(*m_resource, dsvDesc);
			it = m_dsvSlices.find(slice);
		}

		return *it->second;
	}

	const BFShaderResourceView& BFTexture::SRV()
	{
		BFSRVTextureViewInfo info;
		info.FirstSlice = 0;
		info.LastSlice = m_desc.ArraySize - 1;
		info.MipSlice = 0;
		info.SRGB = m_desc.SRGBHint;
		return SRV(info);
	}

	const BFShaderResourceView& BFTexture::SRV(const BFSRVTextureViewInfo& info)
	{
		BF_CORE_ASSERT(info.FirstSlice < m_desc.ArraySize, "Slice index out of bounds for SRV array slice.");

		const uint32_t lastSlice = std::min(info.LastSlice, m_desc.ArraySize - 1);

		uint64_t key = 1;
		Utils::SumHash(key, info.FirstSlice);
		Utils::SumHash(key, lastSlice);
		Utils::SumHash(key, info.MipSlice);
		Utils::SumHash(key, static_cast<uint64_t>(info.SRGB + 1));

		auto it = m_srvSlices.find(key);

		if (it == m_srvSlices.end())
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = Utils::CreateSrvFromHWTextureDesc(m_desc, info);
			m_srvSlices[key] = MakeRef<BFShaderResourceView>(*m_resource, srvDesc);
			it = m_srvSlices.find(key);
		}

		return *it->second;
	}

	const BFUnorderedAccessView& BFTexture::UAV(const BFUAVTextureViewInfo& info)
	{
		BF_CORE_ASSERT(info.FirstSlice < m_desc.ArraySize, "Slice index out of bounds for UAV array slice.");

		const uint32_t lastSlice = std::min(info.LastSlice, m_desc.ArraySize - 1);
		uint64_t key = 1;
		Utils::SumHash(key, info.FirstSlice);
		Utils::SumHash(key, lastSlice);
		Utils::SumHash(key, info.MipSlice);

		auto it = m_uavSlices.find(key);

		if (it == m_uavSlices.end())
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = Utils::CreateUavFromHWTextureDesc(m_desc, info);
			m_uavSlices[key] = MakeRef<BFUnorderedAccessView>(*m_resource, uavDesc);
			it = m_uavSlices.find(key);
		}

		return *it->second;
	}

	const BFUnorderedAccessView& BFTexture::UAV()
	{
		BFUAVTextureViewInfo info;
		info.FirstSlice = 0;
		info.LastSlice = m_desc.ArraySize - 1;
		info.MipSlice = 0;
		return UAV(info);
	}

	BFTextureReadback::BFTextureReadback()
	{
		m_readbackBuffer = DX12ResourceBuilder()
			.HeapType(D3D12_HEAP_TYPE_READBACK)
			.InitialState(D3D12_RESOURCE_STATE_COPY_DEST)
			.Buffer(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT)
			.SetName("Object Picker Pixel Readback")
			.Create();
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
		ThrowIfFailed(m_readbackBuffer->HwResource->Map(0, &readRange, &mapped));
		out = *static_cast<const uint32_t*>(mapped);

		D3D12_RANGE writtenRange{ 0, 0 };
		m_readbackBuffer->HwResource->Unmap(0, &writtenRange);

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

		src->Transition(list, D3D12_RESOURCE_STATE_COPY_SOURCE);


		D3D12_TEXTURE_COPY_LOCATION source{};
		source.pResource = src->HwResource;
		source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		source.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION destination{};
		destination.pResource = m_readbackBuffer->HwResource;
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
