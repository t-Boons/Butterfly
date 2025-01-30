#pragma once
#include "Common_DX12.hpp"

#define BUFFER_COUNT 2

namespace Butterfly
{
	class DX12Resource;
	class DX12CommandList;
	class DX12Fence;

	class BFGraphicsContext : private NonCopyable
	{
	public:
		BFGraphicsContext(HWND hwnd);
		~BFGraphicsContext();

		void DisplayTexture(DX12Resource& toDisplay);
		void Resize(uint32_t width, uint32_t height);

		bool VSync;

	public:
		void CreateContext();
		void FreeContext();

		IDXGISwapChain4* m_swapChain;
		DX12CommandList* m_swapchainCmdList[BUFFER_COUNT];
		DX12Fence* m_fence;
		DX12Resource* m_renderTargets[BUFFER_COUNT];
		bool m_ctxInitialized;

		HWND m_hwnd;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_frameIndex;
		uint32_t m_backBufferIndex;
	};
}