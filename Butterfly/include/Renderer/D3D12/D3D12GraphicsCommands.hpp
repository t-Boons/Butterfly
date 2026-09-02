#pragma once
#include "D3D12Common.hpp"

namespace Butterfly
{
	class D3D12CommandList;
	class BFTexture;

	class GraphicsCommands
	{
	public:
		static void SetRenderTargets(D3D12CommandList& list, const std::vector<BFTexture*>& rts, BFTexture* dsv);

		static void ClearRenderTarget(D3D12CommandList& list, BFTexture& rt);
		static void ClearRenderTarget(D3D12CommandList& list, BFTexture& rt, const std::array<float, 4>& color);

		static void ClearDepthStencil(D3D12CommandList& list, BFTexture& ds);

		static void SetFullscreenViewportAndRect(D3D12CommandList& list, uint32_t width, uint32_t height);
		static void SetBindlessDescriptorHeapsAndRootSignature(D3D12CommandList& list);
	};
}
