#pragma once
#include "D3D12Common.hpp"

#define BUFFER_COUNT 2

namespace Butterfly
{
	class D3D12Resource;
	class D3D12CommandList;
	class D3D12Fence;

	class BFGraphicsContext : private NonCopyable
	{
	public:
		BFGraphicsContext(HWND hwnd);
		~BFGraphicsContext();

		void DisplayTexture(D3D12Resource& toDisplay);
		void Resize(uint32_t width, uint32_t height);

		bool VSync;

	public:
		void CreateContext();
		void FreeContext();

		IDXGISwapChain4* m_swapChain;
		D3D12CommandList* m_swapchainCmdList[BUFFER_COUNT];
		D3D12Resource* m_renderTargets[BUFFER_COUNT];
		bool m_ctxInitialized;

		HWND m_hwnd;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_frameIndex;
		uint32_t m_backBufferIndex;
	};
}
