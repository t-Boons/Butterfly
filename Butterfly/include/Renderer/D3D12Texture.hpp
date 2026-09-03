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
		static RefPtr<BFTexture> CreateTextureFromCPUBuffer(const BFTextureDesc& desc, const void* data, const std::string& resourceTag);
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
}
