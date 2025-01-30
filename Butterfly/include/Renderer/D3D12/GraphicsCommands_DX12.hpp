#pragma once
#include "Common_DX12.hpp"

namespace Butterfly
{
	class DX12CommandList;
	class BFTexture;

	class GraphicsCommands
	{
	public:
		static void SetRenderTargets(DX12CommandList& list, const std::vector<BFTexture*>& rts, BFTexture* dsv);

		static void ClearRenderTarget(DX12CommandList& list, BFTexture& rt);
		static void ClearRenderTarget(DX12CommandList& list, BFTexture& rt, const std::array<float, 4>& color);

		static void ClearDepthStencil(DX12CommandList& list, BFTexture& ds);

		static void SetFullscreenViewportAndRect(DX12CommandList& list, uint32_t width, uint32_t height);
		static void SetBindlessDescriptorHeapsAndRootSignature(DX12CommandList& list);
	};
}