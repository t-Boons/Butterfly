#pragma once
#include "D3D12/D3D12Common.hpp"
#include "Resource.hpp"

namespace Butterfly
{
	class D3D12Resource;
	class BFDepthStencilView;
	class BFRenderTargetView;
	class BFShaderResourceView;
	class BFUnorderedAccessView;

	enum class BFTextureType
	{
		Texture2D,
		Texture2DArray,
		Cubemap,
		Texture3D,
	};

	class BFTexture;
	struct BFTextureUploadData
	{
		bool HasValidFace() const { return std::any_of(CubemapFacesSource.begin(), CubemapFacesSource.end(), [](const RefPtr<BFTexture>& face) { return face != nullptr; }); }

		std::array<RefPtr<BFTexture>, 6> CubemapFacesSource;
		RefPtr<BFTexture> GPUCopySource = nullptr;
		const void* CPUCopySource = nullptr;
	};

	struct BFTextureDesc
	{
		BFTextureType Type = BFTextureType::Texture2D;
		BFTextureUploadData UploadData;
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t Depth = 1;
		uint32_t ArraySize = 1;
		uint32_t ViewTypes = None;
		std::string DebugName = "Texture";
		bool SRGB = false;
		uint32_t NumMips = 1; // 0 Means generate all.
		//bool GenerateMips = false;

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

		BFTextureDesc Desc() const { return m_desc; }
		DXGI_FORMAT Format() const { return m_desc.Format; }
		const D3D12_CLEAR_VALUE* ClearValue() const;

		const BFDepthStencilView& DSV();
		const BFDepthStencilView& DSVArraySlice(uint32_t slice);
		const BFRenderTargetView& RTV() const;
		const BFShaderResourceView& SRV();
		const BFShaderResourceView& SRVArraySlice(uint32_t firstSlice, uint32_t lastSlice);
		const BFUnorderedAccessView& UAV() const;

		uint32_t Width() const { return m_desc.Width; }
		uint32_t Height() const { return m_desc.Height; }
		uint32_t Depth() const { return m_desc.Depth; }
		uint32_t NumMips() const { return m_desc.NumMips; }
		uint32_t ViewTypes() const { return m_desc.ViewTypes; }
		BFTextureType Type() const { return m_desc.Type; }
		D3D12Resource* Resource() { return m_resource; }
		const D3D12Resource* Resource() const { return m_resource; }

	private:
		D3D12Resource* m_resource = nullptr;
		BFTextureDesc m_desc{};
		BFRenderTargetView* m_rtv = nullptr;
		BFUnorderedAccessView* m_uav = nullptr;

		std::unordered_map<uint64_t, RefPtr<BFShaderResourceView>> m_srvSlices;
		std::unordered_map<uint32_t, RefPtr<BFDepthStencilView>> m_dsvSlices;
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
