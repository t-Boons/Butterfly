#pragma once
#include "D3D12Common.hpp"

namespace Butterfly
{
	class D3D12CommandList;
	class BFTexture;

	class GraphicsCommands
	{
	public:
		static void Blit(D3D12CommandList& list, BFTexture& src, BFTexture& dst);

		static void SetFullscreenViewportAndRect(D3D12CommandList& list, uint32_t width, uint32_t height);
		static void SetBindlessDescriptorHeapsAndRootSignature(D3D12CommandList& list);
	};
}
