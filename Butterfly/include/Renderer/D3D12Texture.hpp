#pragma once
#include "D3D12/D3D12Common.hpp"
#include "Resource.hpp"

namespace Butterfly
{
	class D3D12Resource;
	class BFDepthStencilView;
	class BFRenderTargetView;
	class BFShaderResourceView;

	struct BFTextureDesc
	{
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t Flags = None;
		std::string DebugName = "Texture";

		enum Flag : uint32_t
		{
			None = 1 << 0,
			ShaderResource = 1 << 1,
			RenderTargettable = 1 << 2,
			DepthStencilable = 1 << 3,
		};
	};

	class BFTexture : public BFResource, private NonCopyable
	{
	public:
		static RefPtr<BFTexture> CreateTextureFromCPUBuffer(const BFTextureDesc& desc, const void* data);
		static RefPtr<BFTexture> CreateTextureForGPU(const BFTextureDesc& desc);

		~BFTexture();

		BFTextureDesc Desc() const { return m_desc; }

		const D3D12_CLEAR_VALUE* ClearValue() const;

		const BFDepthStencilView& DSV() const;
		const BFRenderTargetView& RTV() const;
		const BFShaderResourceView& SRV() const;

		uint32_t Width() const { return m_desc.Width; }
		uint32_t Height() const { return m_desc.Height; }

		D3D12Resource* Resource() { return m_resource; }

	private:
		BFTexture() = default;
		void CreateViews(const BFTextureDesc& desc);

		D3D12Resource* m_resource = nullptr;
		BFTextureDesc m_desc{};
		BFDepthStencilView* m_dsv = nullptr;
		BFRenderTargetView* m_rtv = nullptr;
		BFShaderResourceView* m_srv = nullptr;
	};

	class BFTextureReadback
	{
	public:
		BFTextureReadback(const RefPtr<BFTexture>& texture)
			: m_texture(texture)
		{
			const auto desc = m_texture->Resource()->HwResource->GetDesc();

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

			m_readbackBuffer = DX12ResourceBuilder()
				.HeapType(D3D12_HEAP_TYPE_READBACK)
				.Buffer(totalSize)
				.SetName("ReadbackBuffer")
				.Create();
		}

		bool IsReady() const
		{
			return m_isReady;
		}

		void Reset()
		{
			m_isReady = false;
		}

		void MarkReady()
		{
			m_isReady = true;
		}

		void ReadbackCopy(D3D12CommandList& list)
		{
			D3D12Resource* src = m_texture->Resource();

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

	private:
		RefPtr<BFTexture> m_texture;
		D3D12Resource* m_readbackBuffer = nullptr;
		bool m_isReady = false;

		uint64_t m_fenceValue = 0;

		uint32_t m_rowPitch = 0;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
	};
}
