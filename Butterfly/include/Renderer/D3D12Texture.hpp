#pragma once
#include "D3D12/D3D12Common.hpp"
#include "Renderer/D3D12/D3D12SubresourceRange.hpp"
#include "Resource.hpp"

namespace Butterfly
{
	class D3D12Resource;
	class BFDepthStencilView;
	class BFRenderTargetView;
	class BFShaderResourceView;
	class BFUnorderedAccessView;

	enum class BFTextureDimention
	{
		Texture2D,
		Texture3D,
	};

	enum class BFTextureUsage
	{
		Texture2D,
		Texture2DArray,
		Cubemap,
		Texture3D,
	};

	struct BFSRVTextureViewInfo
	{
		BFTextureUsage Type = BFTextureUsage::Texture2D;
		D3D12SubresourceRange Range{};
		bool SRGB = false;

		static BFSRVTextureViewInfo Texture2D()      { return { BFTextureUsage::Texture2D }; }
		static BFSRVTextureViewInfo Texture2DArray() { return { BFTextureUsage::Texture2DArray }; }
		static BFSRVTextureViewInfo Texture3D()      { return { BFTextureUsage::Texture3D }; }
		static BFSRVTextureViewInfo Cubemap()        { return { BFTextureUsage::Cubemap }; }

		BFSRVTextureViewInfo& Mips(uint32_t first, uint32_t count)   { Range.MipRange(first, count); return *this; }
		BFSRVTextureViewInfo& Slices(uint32_t first, uint32_t count) { Range.SliceRange(first, count); return *this; }
		BFSRVTextureViewInfo& AsSRGB(bool srgb = true)               { SRGB = srgb; return *this; }
	};

	struct BFUAVTextureViewInfo
	{
		BFTextureUsage Type = BFTextureUsage::Texture2D;
		D3D12SubresourceRange Range = D3D12SubresourceRange().MipRange(0, 1);

		static BFUAVTextureViewInfo Texture2D()      { return { BFTextureUsage::Texture2D }; }
		static BFUAVTextureViewInfo Texture2DArray() { return { BFTextureUsage::Texture2DArray }; }
		static BFUAVTextureViewInfo Texture3D()      { return { BFTextureUsage::Texture3D }; }

		BFUAVTextureViewInfo& Mip(uint32_t mip)                      { Range.MipRange(mip, 1); return *this; }
		BFUAVTextureViewInfo& Slices(uint32_t first, uint32_t count) { Range.SliceRange(first, count); return *this; }
	};

	struct BFRTVTextureViewInfo
	{
		BFTextureUsage Type = BFTextureUsage::Texture2D;
		D3D12SubresourceRange Range = D3D12SubresourceRange().MipRange(0, 1);
		bool SRGB = false;

		static BFRTVTextureViewInfo Texture2D()      { return { BFTextureUsage::Texture2D }; }
		static BFRTVTextureViewInfo Texture2DArray() { return { BFTextureUsage::Texture2DArray }; }
		static BFRTVTextureViewInfo Texture3D()      { return { BFTextureUsage::Texture3D }; }

		BFRTVTextureViewInfo& Mip(uint32_t mip)                      { Range.MipRange(mip, 1); return *this; }
		BFRTVTextureViewInfo& Slices(uint32_t first, uint32_t count) { Range.SliceRange(first, count); return *this; }
		BFRTVTextureViewInfo& AsSRGB(bool srgb = true)               { SRGB = srgb; return *this; }
	};

	struct BFDSVTextureViewInfo
	{
	public:
		BFTextureUsage Type = BFTextureUsage::Texture2D;
		D3D12SubresourceRange Range = D3D12SubresourceRange().MipRange(0, 1);

		static BFDSVTextureViewInfo Texture2D()      { return { BFTextureUsage::Texture2D }; }
		static BFDSVTextureViewInfo Texture2DArray() { return { BFTextureUsage::Texture2DArray }; }

		BFDSVTextureViewInfo& Mip(uint32_t mip)                      { Range.MipRange(mip, 1); return *this; }
		BFDSVTextureViewInfo& Slices(uint32_t first, uint32_t count) { Range.SliceRange(first, count); return *this; }
	};

	struct BFTextureDesc
	{
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t Depth = 1;
		uint32_t ArraySize = 1;
		uint32_t ViewTypes = None;
		BFTextureDimention Dimention = BFTextureDimention::Texture2D;
		std::string DebugName = "Texture";
		bool IsSRGB = false;
		uint32_t NumMips = 1; // BF_ALL Means generate all.

		enum ViewType : uint32_t
		{
			None = 1 << 0,
			ShaderResource = 1 << 1,
			RenderTargettable = 1 << 2,
			DepthStencilable = 1 << 3,
			UnorderedAccess = 1 << 4,
		};
	};

	class BFTexture : public BFResource, private NonCopyable
	{
	public:
		BFTexture(const BFTextureDesc& desc);
		~BFTexture();

		const BFTextureDesc& Desc() const { return m_desc; }
		DXGI_FORMAT Format() const { return m_desc.Format; }
		D3D12_CLEAR_VALUE ClearValue() const;

		const BFDepthStencilView& DSV(BFDSVTextureViewInfo info);
		const BFRenderTargetView& RTV(BFRTVTextureViewInfo info);
		const BFShaderResourceView& SRV(BFSRVTextureViewInfo info);
		const BFUnorderedAccessView& UAV(BFUAVTextureViewInfo info);

		uint32_t Width() const { return m_desc.Width; }
		uint32_t Height() const { return m_desc.Height; }
		uint32_t Depth() const { return m_desc.Depth; }
		uint32_t ArraySize() const { return m_desc.ArraySize; }
		uint32_t NumMips() const { return m_desc.NumMips; }
		uint32_t ViewTypes() const { return m_desc.ViewTypes; }
		D3D12Resource* Resource() { return m_resource; }
		const D3D12Resource* Resource() const { return m_resource; }

		bool IsSRGB() const { return m_desc.IsSRGB; }

	private:
		D3D12Resource* m_resource = nullptr;
		BFTextureDesc m_desc{};

		std::unordered_map<uint64_t, RefPtr<BFShaderResourceView>> m_srvSlices;
		std::unordered_map<uint32_t, RefPtr<BFDepthStencilView>> m_dsvSlices;
		std::unordered_map<uint64_t, RefPtr<BFUnorderedAccessView>> m_uavSlices;
		std::unordered_map<uint64_t, RefPtr<BFRenderTargetView>> m_rtvSlices;
	};


	class D3D12CommandList;

	class BFTextureReadback
	{
	public:
		BFTextureReadback();
		~BFTextureReadback();

		bool Readback(D3D12CommandList& list, BFTexture& texture);
		bool RequestPixel(const glm::uvec2& pixel);

		void SetFenceValue(uint64_t fenceValue);

		bool TryGetResult(uint64_t fenceValue, uint32_t& outValue);

		void Flush();

	private:
		D3D12Resource* m_readbackBuffer = nullptr;
		uint64_t m_fenceValue = 0;

		bool m_hasRequest = false;
		bool m_waitingForResult = false;
		bool m_copyRecorded = false;
		glm::uvec2 m_requestedPixel;
	};
}
