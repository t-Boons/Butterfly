#include "Renderer/Texture_DX12.hpp"
#include "Renderer/D3D12/GraphicsAPI_DX12.hpp"
#include "Renderer/D3D12/CommandQueue_DX12.hpp"
#include "Renderer/D3D12/CommandList_DX12.hpp"
#include "Renderer/D3D12/Resource_DX12.hpp"
#include "Renderer/D3D12/View_DX12.hpp"

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
			return srvDesc;
		}
	}

	// Texture create functions.

	BFTexture* BFTexture::CreateTextureFromCPUBuffer(const BFTextureDesc& desc, const void* data, const std::string& resourceTag)
	{
		BF_PROFILE_EVENT("BFTexture::BFTexture (Texture upload)");

		BFTexture* newTexture = new BFTexture();

		newTexture->m_resource = DX12ResourceBuilder()
			.HeapType(D3D12_HEAP_TYPE_DEFAULT)
			.InitialState(D3D12_RESOURCE_STATE_COPY_DEST)
			.Texture2D(desc.Format, desc.Width, desc.Height)
			.SetName(resourceTag)
			.Create();


		UINT64 totalSize = 0;
		UINT64 rowPitch = 0;
		UINT numRows = 0;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		D3D12_RESOURCE_DESC textureDesc = newTexture->m_resource->HwResource->GetDesc();
		DX12API()->Device()->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footprint, &numRows, &rowPitch, &totalSize);


		DX12Resource* uploadResource = DX12ResourceBuilder()
			.HeapType(D3D12_HEAP_TYPE_UPLOAD)
			.InitialState(D3D12_RESOURCE_STATE_COPY_SOURCE)
			.Buffer(totalSize)
			.SetName("Intermediate texture.")
			.Create()
			->Write(data, static_cast<uint32_t>(totalSize));


		D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
		srcLocation.pResource = uploadResource->HwResource;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		srcLocation.PlacedFootprint = footprint;

		D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
		dstLocation.pResource = newTexture->m_resource->HwResource;
		dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dstLocation.SubresourceIndex = 0;

		{
			DX12CommandList list(D3D12_COMMAND_LIST_TYPE_COPY);

			list.List()->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

			list.Close();
			DX12API()->Queue(QueueType::Copy)->Execute(list);
			DX12API()->Queue(QueueType::Copy)->WaitForFence();
		}

		{
			DX12CommandList list;

			newTexture->m_resource->Transition(list, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			list.Close();
			DX12API()->Queue(QueueType::Direct)->Execute(list);
			DX12API()->Queue(QueueType::Direct)->WaitForFence();
		}

		delete uploadResource;
		newTexture->CreateViews(desc);

		return newTexture;
	}

	BFTexture* BFTexture::CreateTextureForGPU(const BFTextureDesc& desc)
	{
		BF_PROFILE_EVENT();

		BFTexture* newTexture = new BFTexture();

		CheckMsg(!(desc.Flags & BFTextureDesc::RenderTargettable &&
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
				.Texture2D(desc.Format, desc.Width, desc.Height)
				.InitialState(D3D12_RESOURCE_STATE_GENERIC_READ)
				.SetName(desc.DebugName)
				.Create();

			newTexture->CreateViews(desc);
		}

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
		CheckMsg(m_rtv != nullptr, "%s", "Texture does not have RTV, the BFTexture::Desc::Flag needs to have BFTexture::Desc::RenderTargettable set.");
		return *m_rtv;
	}

	const BFDepthStencilView& BFTexture::DSV() const
	{
		CheckMsg(m_dsv != nullptr, "%s", "Texture does not have DSV, the BFTexture::Desc::Flag needs to have BFTexture::Desc::DepthStencilable set.");
		return *m_dsv;
	}

	const BFShaderResourceView& BFTexture::SRV() const
	{
		CheckMsg(m_srv != nullptr, "%s", "Texture does not have SRV, the BFTexture::Desc::Flag needs to have BFTexture::Desc::ShaderResource set.");
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
}