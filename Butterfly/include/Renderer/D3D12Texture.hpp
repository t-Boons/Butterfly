#pragma once
#include "D3D12/D3D12Common.hpp"
#include "Renderer/D3D12/D3D12SubresourceRange.hpp"
#include "Resource.hpp"

namespace Butterfly
{
	class D3D12Resource;
	class DepthStencilView;
	class RenderTargetView;
	class ShaderResourceView;
	class UnorderedAccessView;

	enum class TextureDimension
	{
		Texture2D,
		Texture3D,
	};

	enum class TextureUsage
	{
		Texture2D,
		Texture2DArray,
		Cubemap,
		Texture3D,
	};

	struct SRVTextureViewInfo
	{
		TextureUsage Type = TextureUsage::Texture2D;
		D3D12SubresourceRange Range{};
		bool SRGB = false;

		static SRVTextureViewInfo Texture2D()      { return { TextureUsage::Texture2D }; }
		static SRVTextureViewInfo Texture2DArray() { return { TextureUsage::Texture2DArray }; }
		static SRVTextureViewInfo Texture3D()      { return { TextureUsage::Texture3D }; }
		static SRVTextureViewInfo Cubemap()        { return { TextureUsage::Cubemap }; }

		SRVTextureViewInfo& Mips(uint32_t first, uint32_t count)   { Range.MipRange(first, count); return *this; }
		SRVTextureViewInfo& Slices(uint32_t first, uint32_t count) { Range.SliceRange(first, count); return *this; }
		SRVTextureViewInfo& AsSRGB(bool srgb = true)               { SRGB = srgb; return *this; }
	};

	struct UAVTextureViewInfo
	{
		TextureUsage Type = TextureUsage::Texture2D;
		D3D12SubresourceRange Range = D3D12SubresourceRange().MipRange(0, 1);

		static UAVTextureViewInfo Texture2D()      { return { TextureUsage::Texture2D }; }
		static UAVTextureViewInfo Texture2DArray() { return { TextureUsage::Texture2DArray }; }
		static UAVTextureViewInfo Texture3D()      { return { TextureUsage::Texture3D }; }

		UAVTextureViewInfo& Mip(uint32_t mip)                      { Range.MipRange(mip, 1); return *this; }
		UAVTextureViewInfo& Slices(uint32_t first, uint32_t count) { Range.SliceRange(first, count); return *this; }
	};

	struct RTVTextureViewInfo
	{
		TextureUsage Type = TextureUsage::Texture2D;
		D3D12SubresourceRange Range = D3D12SubresourceRange().MipRange(0, 1);
		bool SRGB = false;

		static RTVTextureViewInfo Texture2D()      { return { TextureUsage::Texture2D }; }
		static RTVTextureViewInfo Texture2DArray() { return { TextureUsage::Texture2DArray }; }
		static RTVTextureViewInfo Texture3D()      { return { TextureUsage::Texture3D }; }

		RTVTextureViewInfo& Mip(uint32_t mip)                      { Range.MipRange(mip, 1); return *this; }
		RTVTextureViewInfo& Slices(uint32_t first, uint32_t count) { Range.SliceRange(first, count); return *this; }
		RTVTextureViewInfo& AsSRGB(bool srgb = true)               { SRGB = srgb; return *this; }
	};

	struct DSVTextureViewInfo
	{
	public:
		TextureUsage Type = TextureUsage::Texture2D;
		D3D12SubresourceRange Range = D3D12SubresourceRange().MipRange(0, 1);

		static DSVTextureViewInfo Texture2D()      { return { TextureUsage::Texture2D }; }
		static DSVTextureViewInfo Texture2DArray() { return { TextureUsage::Texture2DArray }; }

		DSVTextureViewInfo& Mip(uint32_t mip)                      { Range.MipRange(mip, 1); return *this; }
		DSVTextureViewInfo& Slices(uint32_t first, uint32_t count) { Range.SliceRange(first, count); return *this; }
	};

	struct TextureDesc
	{
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t Depth = 1;
		uint32_t ArraySize = 1;
		uint32_t ViewTypes = None;
		TextureDimension Dimension = TextureDimension::Texture2D;
		std::string DebugName = "Texture";
		bool IsSRGB = false;
		uint32_t NumMips = 1; // BF_ALL Means generate all.

		enum ViewType : uint32_t
		{
			None = 1 << 0,
			ShaderResource = 1 << 1,
			RenderTargetable = 1 << 2,
			DepthStencilable = 1 << 3,
			UnorderedAccess = 1 << 4,
		};

		bool operator ==(const TextureDesc& other) const = default;
	};

	class Texture : public Resource, private NonCopyable
	{
	public:
		Texture(const TextureDesc& desc);
		~Texture();

		DXGI_FORMAT Format() const { return m_desc.Format; }
		D3D12_CLEAR_VALUE ClearValue() const;

		const DepthStencilView& DSV(DSVTextureViewInfo info);
		const RenderTargetView& RTV(RTVTextureViewInfo info);
		const ShaderResourceView& SRV(SRVTextureViewInfo info);
		const UnorderedAccessView& UAV(UAVTextureViewInfo info);

		uint32_t Width() const { return m_desc.Width; }
		uint32_t Height() const { return m_desc.Height; }
		uint32_t Depth() const { return m_desc.Depth; }
		uint32_t ArraySize() const { return m_desc.ArraySize; }
		uint32_t NumMips() const { return m_desc.NumMips; }
		uint32_t ViewTypes() const { return m_desc.ViewTypes; }
		D3D12Resource* Resource() { return m_resource; }
		const D3D12Resource* Resource() const { return m_resource; }

		bool IsSRGB() const { return m_desc.IsSRGB; }
		const std::string& DebugName() const { return m_desc.DebugName; }

	private:
		D3D12Resource* m_resource = nullptr;
		TextureDesc m_desc{};

		std::unordered_map<uint64_t, RefPtr<ShaderResourceView>> m_srvSlices;
		std::unordered_map<uint32_t, RefPtr<DepthStencilView>> m_dsvSlices;
		std::unordered_map<uint64_t, RefPtr<UnorderedAccessView>> m_uavSlices;
		std::unordered_map<uint64_t, RefPtr<RenderTargetView>> m_rtvSlices;
	};


	class D3D12CommandList;

	class TextureReadback
	{
	public:
		TextureReadback();
		~TextureReadback();

		bool Readback(D3D12CommandList& list, Texture& texture);
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
