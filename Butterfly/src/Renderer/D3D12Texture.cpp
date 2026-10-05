#include "Renderer/D3D12Texture.hpp"
#include "Renderer/D3D12/D3D12GraphicsAPI.hpp"
#include "Renderer/D3D12/D3D12CommandQueue.hpp"
#include "Renderer/D3D12/D3D12CommandList.hpp"
#include "Renderer/D3D12/D3D12Resource.hpp"
#include "Renderer/D3D12/D3D12View.hpp"

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

			if (desc.SRGB)
			{
				rtvDesc.Format = GetSRGBFormat(rtvDesc.Format);
			}


			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			return rtvDesc;
		}

		inline const D3D12_DEPTH_STENCIL_VIEW_DESC CreateDsvDescFromHWTextureDesc(const BFTextureDesc& desc)
		{
			BF_PROFILE_EVENT();

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};

			dsvDesc.Format = desc.Format;
			if (desc.SRGB)
			{
				dsvDesc.Format = GetSRGBFormat(dsvDesc.Format);
			}

			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			return dsvDesc;
		}

		inline const D3D12_SHADER_RESOURCE_VIEW_DESC CreateSrvFromHWTextureDesc(const BFTextureDesc& desc)
		{
			BF_PROFILE_EVENT();

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = desc.Format;
			if (desc.SRGB)
			{
				srvDesc.Format = GetSRGBFormat(srvDesc.Format);
			}

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.PlaneSlice = 0;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

			if (desc.Type == BFTextureType::Texture2DArray)
			{
				BF_CORE_ASSERT(desc.ArraySize > 1, "Texture2DArray must have ArraySize > 1.");
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.Texture2DArray.ArraySize = desc.ArraySize;
				srvDesc.Texture2DArray.FirstArraySlice = 0;
				srvDesc.Texture2DArray.MostDetailedMip = 0;
				srvDesc.Texture2DArray.MipLevels = 1;
			}

			if (desc.Type == BFTextureType::Cubemap)
			{
				BF_CORE_ASSERT(desc.ArraySize == 6, "Cubemap must have 6 faces.");
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MostDetailedMip = 0;
				srvDesc.TextureCube.MipLevels = 1;
				srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
			}
			if (desc.Type == BFTextureType::Texture3D)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				srvDesc.Texture3D.MostDetailedMip = 0;
				srvDesc.Texture3D.MipLevels = 1;
				srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
			}

			return srvDesc;
		}

		inline const D3D12_UNORDERED_ACCESS_VIEW_DESC CreateUavFromHWTextureDesc(const BFTextureDesc& desc)
		{
			BF_PROFILE_EVENT();
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = desc.Format;
			if (desc.SRGB)
			{
				uavDesc.Format = GetSRGBFormat(uavDesc.Format);
			}
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = 0;
			uavDesc.Texture2D.PlaneSlice = 0;
			if (desc.Type == BFTextureType::Texture2DArray)
			{
				BF_CORE_ASSERT(desc.ArraySize > 1, "Texture2DArray must have ArraySize > 1.");
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2DArray.ArraySize = desc.ArraySize;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;
			}
			if (desc.Type == BFTextureType::Cubemap)
			{
				BF_CORE_ASSERT(desc.ArraySize == 6, "Cubemap must have 6 faces.");
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2DArray.ArraySize = desc.ArraySize;
				uavDesc.Texture2DArray.FirstArraySlice = 0;
				uavDesc.Texture2DArray.MipSlice = 0;
			}
			if (desc.Type == BFTextureType::Texture3D)
			{
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				uavDesc.Texture3D.MipSlice = 0;
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
			BF_CORE_ASSERT(m_desc.Type != BFTextureType::Cubemap, "DepthStencilable textures cannot be Cubemaps.");
			BF_CORE_ASSERT(m_desc.Type != BFTextureType::Texture2DArray, "DepthStencilable textures cannot be Texture2DArray.");
			BF_CORE_ASSERT(m_desc.ArraySize == 1, "DepthStencilable textures cannot be Texture2DArray.");

			float col[] = { 0,0,0,0 };
			m_resource = DX12ResourceBuilder()
				.HeapType(D3D12_HEAP_TYPE_DEFAULT)
				.DepthStencil(m_desc.Format, m_desc.Width, m_desc.Height)
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

			if (m_desc.ViewTypes & BFTextureDesc::ViewType::UnorderedAccess)
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
		if (m_desc.ViewTypes & BFTextureDesc::ViewType::DepthStencilable)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = Utils::CreateDsvDescFromHWTextureDesc(m_desc);
			m_dsv = new BFDepthStencilView(*m_resource, dsvDesc);
		}
		if (m_desc.ViewTypes & BFTextureDesc::ViewType::ShaderResource)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = Utils::CreateSrvFromHWTextureDesc(m_desc);
			m_srv = new BFShaderResourceView(*m_resource, srvDesc);
		}
		if (m_desc.ViewTypes & BFTextureDesc::ViewType::UnorderedAccess)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = Utils::CreateUavFromHWTextureDesc(m_desc);
			m_uav = new BFUnorderedAccessView(*m_resource, uavDesc);
		}
	}

	BFTexture::~BFTexture()
	{
		BF_PROFILE_EVENT();

		FREE(m_rtv);
		FREE(m_dsv);
		FREE(m_srv);
		FREE(m_uav);
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

	const BFDepthStencilView& BFTexture::DSV() const
	{
		BF_CORE_ASSERT(m_dsv != nullptr, "%s", "Texture does not have DSV, the BFTexture::Desc::Flag needs to have BFTexture::Desc::DepthStencilable set.");
		return *m_dsv;
	}

	const BFShaderResourceView& BFTexture::SRV() const
	{
		BF_CORE_ASSERT(m_srv != nullptr, "%s", "Texture does not have SRV, the BFTexture::Desc::Flag needs to have BFTexture::Desc::ShaderResource set.");
		return *m_srv;
	}

	const BFUnorderedAccessView& BFTexture::UAV() const
	{
		BF_CORE_ASSERT(m_uav != nullptr, "%s", "Texture does not have UAV, the BFTexture::Desc::Flag needs to have BFTexture::Desc::UnorderedAccess set.");
		return *m_uav;
	}

	BFTextureReadback::BFTextureReadback()
	{

	}

	BFTextureReadback::~BFTextureReadback()
	{
		BF_PROFILE_EVENT();
		FREE(m_readbackBuffer);
	}

	void BFTextureReadback::ValidateBuffer(const RefPtr<BFTexture>& texture)
	{
		BF_PROFILE_EVENT();

		D3D12Resource* src = texture->Resource();
		const auto desc = src->HwResource->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
		UINT numRows = 0;
		UINT64 rowSize = 0;
		UINT64 totalSize = 0;
		D3D12API()->Device()->GetCopyableFootprints(
			&desc,
			0,
			1,
			0,
			&footprint,
			&numRows,
			&rowSize,
			&totalSize
		);

		if (m_readbackBuffer && m_rowPitch == footprint.Footprint.RowPitch && m_width == desc.Width && m_height == desc.Height)
		{
			return;
		}

		FREE(m_readbackBuffer);

		m_readbackBuffer = DX12ResourceBuilder()
			.HeapType(D3D12_HEAP_TYPE_READBACK)
			.InitialState(D3D12_RESOURCE_STATE_COPY_DEST)
			.Buffer(totalSize)
			.SetName("Readback Buffer")
			.Create();

		m_totalSize = static_cast<uint32_t>(totalSize);
		m_rowPitch = static_cast<uint32_t>(footprint.Footprint.RowPitch);
		m_width = static_cast<uint32_t>(desc.Width);
		m_height = static_cast<uint32_t>(desc.Height);
	}

	bool BFTextureReadback::ReadPixel(const glm::ivec2& pixel, uint32_t& out)
	{
		if (!m_readbackBuffer || pixel.x < 0 || pixel.y < 0 || pixel.x >= m_width || pixel.y >= m_height)
		{
			return false;
		}

		const uint8_t* data = static_cast<const uint8_t*>(m_readbackBuffer->Map());
		const uint8_t* row = data + m_rowPitch * pixel.y;
		const uint32_t* pixelData = reinterpret_cast<const uint32_t*>(row) + pixel.x;

		out = *pixelData;

		m_readbackBuffer->Unmap();
		return true;
	}

	void BFTextureReadback::ReadbackCopy(D3D12CommandList& list, const RefPtr<BFTexture>& texture)
	{
		ValidateBuffer(texture);

		D3D12Resource* src = texture->Resource();

		m_textureDesc = texture->Desc();

		const auto desc = src->HwResource->GetDesc();

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
		UINT numRows = 0;
		UINT64 rowSize = 0;
		UINT64 totalSize = 0;

		D3D12API()->Device()->GetCopyableFootprints(
			&desc,
			0,
			1,
			0,
			&footprint,
			&numRows,
			&rowSize,
			&totalSize
		);


		src->Transition(list, D3D12_RESOURCE_STATE_COPY_SOURCE);


		D3D12_TEXTURE_COPY_LOCATION source{};
		source.pResource = src->HwResource;
		source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		source.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION destination{};
		destination.pResource = m_readbackBuffer->HwResource;
		destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		destination.PlacedFootprint = footprint;

		list.List()->CopyTextureRegion(
			&destination,
			0, 0, 0,
			&source,
			nullptr
		);
	}
}
