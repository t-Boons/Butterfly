#pragma once
#include "D3D12/D3D12Common.hpp"
#include "Resource.hpp"

namespace Butterfly
{
	class D3D12Resource;
	class BFDepthStencilView;
	class BFRenderTargetView;
	class BFShaderResourceView;

	enum class BFTextureType
	{
		Texture2D,
		Texture2DArray,
		Cubemap,
	};

	struct BFTextureDesc
	{
		BFTextureType Type = BFTextureType::Texture2D;
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t ArraySize = 1;
		uint32_t Flags = None;
		std::string DebugName = "Texture";
		const void* Data = nullptr;

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
		static RefPtr<BFTexture> CreateTextureFromCPUBuffer(const BFTextureDesc& desc);
		static RefPtr<BFTexture> CreateTextureForGPU(const BFTextureDesc& desc);

		static RefPtr<BFTexture> CreateCubemap(const std::array<RefPtr<BFTexture>, 6>& textures);

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


	class D3D12CommandList;

	class BFTextureReadback
	{
	public:
		BFTextureReadback();
		~BFTextureReadback();

		bool ReadPixel(const glm::ivec2& pixel, uint32_t& out);

		void ReadbackCopy(D3D12CommandList& list, const RefPtr<BFTexture>& texture);

	private:
		void ValidateBuffer(const RefPtr<BFTexture>& texture);

		BFTextureDesc m_textureDesc;
		D3D12Resource* m_readbackBuffer = nullptr;
		bool m_isReady = false;

		uint64_t m_fenceValue = 0;

		uint32_t m_totalSize = 0;
		uint32_t m_rowPitch = 0;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
	};
}
