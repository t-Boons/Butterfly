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
		inline const D3D12_RENDER_TARGET_VIEW_DESC CreateRTVDescFromHWTextureDesc(const BFTextureDesc& desc)
		{
			BF_PROFILE_EVENT();

			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.Format = desc.Format;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			return rtvDesc;
		}

		inline const D3D12_DEPTH_STENCIL_VIEW_DESC CreateDsvDescFromHWTextureDesc(const BFTextureDesc& desc)
		{
			BF_PROFILE_EVENT();

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = desc.Format;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			return dsvDesc;
		}

		inline const D3D12_SHADER_RESOURCE_VIEW_DESC CreateSrvFromHWTextureDesc(const BFTextureDesc& desc)
		{
			BF_PROFILE_EVENT();

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = desc.Format;
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
			return srvDesc;
		}
	}

	// Texture create functions.

	RefPtr<BFTexture> BFTexture::CreateTextureFromCPUBuffer(const BFTextureDesc& desc)
	{
		BF_PROFILE_EVENT("BFTexture::BFTexture (Texture upload)");

		RefPtr<BFTexture> newTexture = RefPtr<BFTexture>(new BFTexture());

		DX12ResourceBuilder builder;
		builder.HeapType(D3D12_HEAP_TYPE_DEFAULT);
		builder.InitialState(D3D12_RESOURCE_STATE_COMMON);
		builder.Texture(desc.Format, desc.Width, desc.Height, desc.ArraySize);
		builder.SetName(desc.DebugName);

		newTexture->m_resource = builder.Create();

		D3D12_RESOURCE_DESC textureDesc = newTexture->m_resource->HwResource->GetDesc();
		const uint32_t numSubresources = textureDesc.DepthOrArraySize;
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(numSubresources);
		std::vector<UINT> numRows(numSubresources);
		std::vector<UINT64> rowSizes(numSubresources);
		uint64_t totalSize;
		D3D12API()->Device()->GetCopyableFootprints(&textureDesc, 0, numSubresources, 0, footprints.data(), numRows.data(), rowSizes.data(), &totalSize);

		std::vector<D3D12_SUBRESOURCE_DATA> subresources(desc.ArraySize);

		D3D12Resource* uploadResource = DX12ResourceBuilder()
			.HeapType(D3D12_HEAP_TYPE_UPLOAD)
			.InitialState(D3D12_RESOURCE_STATE_COPY_SOURCE)
			.Buffer(totalSize)
			.SetName("Intermediate texture.")
			.Create();

		for (uint32_t slice = 0; slice < desc.ArraySize; ++slice)
		{
			const uint8_t* src = static_cast<const uint8_t*>(desc.Data) + slice * (desc.Width * desc.Height * 4);
			const auto& footprint = footprints[slice];
			uploadResource->Write(src, footprint.Footprint.RowPitch * desc.Height, static_cast<uint32_t>(footprint.Offset));
		}

		D3D12CommandList list(D3D12_COMMAND_LIST_TYPE_COPY);

		for (uint32_t slice = 0; slice < desc.ArraySize; ++slice)
		{
			D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
			srcLocation.pResource = uploadResource->HwResource;
			srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			srcLocation.PlacedFootprint = footprints[slice];

			D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
			dstLocation.pResource = newTexture->m_resource->HwResource;
			dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dstLocation.SubresourceIndex = slice;

			list.List()->CopyTextureRegion(
				&dstLocation,
				0, 0, 0,
				&srcLocation,
				nullptr
			);
		}

		list.Close();
		D3D12API()->Queue(QueueType::Copy)->Execute(list);
		D3D12API()->Queue(QueueType::Copy)->WaitForFence();

		delete uploadResource;
		newTexture->CreateViews(desc);

		return newTexture;
	}

	RefPtr<BFTexture> BFTexture::CreateTextureForGPU(const BFTextureDesc& desc)
	{
		BF_PROFILE_EVENT();

		RefPtr<BFTexture> newTexture = RefPtr<BFTexture>(new BFTexture());

		BF_CORE_ASSERT(!(desc.Flags & BFTextureDesc::RenderTargettable &&
			desc.Flags & BFTextureDesc::DepthStencilable),
			"BFTexture can not have be RenderTargettable and DepthStenillable since it only tracks 1 resource.");


		if (desc.Flags & BFTextureDesc::Flag::RenderTargettable)
		{
			float col[] = { 0,0,0,0 };
			newTexture->m_resource = DX12ResourceBuilder()
				.HeapType(D3D12_HEAP_TYPE_DEFAULT)
				.RenderTarget(desc.Format, desc.Width, desc.Height)
				.ClearColor(col, desc.Format)
				.InitialState(D3D12_RESOURCE_STATE_RENDER_TARGET)
				.SetName(desc.DebugName)
				.Create();

			newTexture->CreateViews(desc);
		} else

		if (desc.Flags & BFTextureDesc::Flag::DepthStencilable)
		{
			float col[] = { 0,0,0,0 };
			newTexture->m_resource = DX12ResourceBuilder()
				.HeapType(D3D12_HEAP_TYPE_DEFAULT)
				.DepthStencil(desc.Format, desc.Width, desc.Height)
				.ClearDepth(desc.Format)
				.InitialState(D3D12_RESOURCE_STATE_DEPTH_READ)
				.SetName(desc.DebugName)
				.Create();

			newTexture->CreateViews(desc);
		} else

		if (desc.Flags & BFTextureDesc::Flag::ShaderResource)
		{
			float col[] = { 0,0,0,0 };
			newTexture->m_resource = DX12ResourceBuilder()
				.HeapType(D3D12_HEAP_TYPE_DEFAULT)
				.Texture(desc.Format, desc.Width, desc.Height, 1)
				.InitialState(D3D12_RESOURCE_STATE_GENERIC_READ)
				.SetName(desc.DebugName)
				.Create();

			newTexture->CreateViews(desc);
		}

		return newTexture;
	}

	RefPtr<BFTexture> BFTexture::CreateCubemap(const std::array<RefPtr<BFTexture>, 6>& textures)
	{
		uint32_t width = 0, height = 0;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

		for (uint32_t i = 0; i < 6; ++i)
		{
			if (!textures[i])
			{
				BF_CORE_LOG_WARN("Cubemap face %d is null.", i);
				continue;
			}
			else
			{
				// Fetch the highest res cubemap texture for now.
				if (width < textures[i]->Desc().Width)
					width = textures[i]->Desc().Width;
				if (height < textures[i]->Desc().Height)
					height = textures[i]->Desc().Height;
				format = textures[i]->Desc().Format;

				break;
			}
		}

		if(format == DXGI_FORMAT_UNKNOWN)
		{
			BF_CORE_LOG_CRITICAL("Cubemap does not have a valid format.");
			return nullptr;
		}

		BFTextureDesc desc;
		desc.ArraySize = 6;
		desc.Type = BFTextureType::Cubemap;
		desc.DebugName = "Cubemap";
		desc.Width = width;
		desc.Height = height;
		desc.Format = format;
		desc.Flags = BFTextureDesc::ShaderResource;

		RefPtr<BFTexture> newTexture = RefPtr<BFTexture>(new BFTexture());

		float col[] = { 0,0,0,0 };
		newTexture->m_resource = DX12ResourceBuilder()
			.HeapType(D3D12_HEAP_TYPE_DEFAULT)
			.Texture(desc.Format, desc.Width, desc.Height, desc.ArraySize)
			.InitialState(D3D12_RESOURCE_STATE_COMMON)
			.SetName("Cubemap")
			.Create();



		D3D12_RESOURCE_DESC textureDesc = newTexture->m_resource->HwResource->GetDesc();
		const uint32_t numSubresources = textureDesc.DepthOrArraySize;
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(numSubresources);
		std::vector<UINT> numRows(numSubresources);
		std::vector<UINT64> rowSizes(numSubresources);
		uint64_t totalSize;
		D3D12API()->Device()->GetCopyableFootprints(&textureDesc, 0, numSubresources, 0, footprints.data(), numRows.data(), rowSizes.data(), &totalSize);

		D3D12CommandList list(D3D12_COMMAND_LIST_TYPE_COPY);

		for (uint32_t slice = 0; slice < desc.ArraySize; ++slice)
		{
			if (!textures[slice])
			{
				continue;
			}

			D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
			srcLocation.pResource = textures[slice]->m_resource->HwResource;
			srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			srcLocation.PlacedFootprint = footprints[slice];
			srcLocation.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
			dstLocation.pResource = newTexture->m_resource->HwResource;
			dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dstLocation.SubresourceIndex = slice;



			list.List()->CopyTextureRegion(
				&dstLocation,
				0, 0, 0,
				&srcLocation,
				nullptr
			);
		}

		list.Close();
		D3D12API()->Queue(QueueType::Copy)->Execute(list);
		D3D12API()->Queue(QueueType::Copy)->WaitForFence();

		newTexture->CreateViews(desc);

		return newTexture;
	}


	// End Texture create functions.



	BFTexture::~BFTexture()
	{
		BF_PROFILE_EVENT();

		FREE(m_rtv);
		FREE(m_dsv);
		FREE(m_srv);
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

	void BFTexture::CreateViews(const BFTextureDesc& desc)
	{
		BF_PROFILE_EVENT();

		m_desc = desc;

		if (desc.Flags & BFTextureDesc::RenderTargettable)
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = Utils::CreateRTVDescFromHWTextureDesc(desc);
			m_rtv = new BFRenderTargetView(*m_resource, rtvDesc);
		}
		if (desc.Flags & BFTextureDesc::Flag::DepthStencilable)
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = Utils::CreateDsvDescFromHWTextureDesc(desc);
			m_dsv = new BFDepthStencilView(*m_resource, dsvDesc);
		}
		if (desc.Flags & BFTextureDesc::Flag::ShaderResource)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = Utils::CreateSrvFromHWTextureDesc(desc);
			m_srv = new BFShaderResourceView(*m_resource, srvDesc);
		}
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
